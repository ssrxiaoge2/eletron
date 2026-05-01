#include "FriendshipModel.h"

#include "../core/Database.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

namespace Backend::Models {
namespace {

QString formatDateTime(const QString& column)
{
    return QStringLiteral("DATE_FORMAT(%1, '%Y-%m-%d %H:%i:%s')").arg(column);
}

int friendStatus(QSqlDatabase db, qint64 currentUserId, qint64 targetUserId)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT user_id, friend_id, status FROM friendships "
        "WHERE ((user_id = :current_user_id AND friend_id = :target_user_id) "
        "OR (user_id = :target_user_id AND friend_id = :current_user_id)) "
        "AND is_deleted = 0 "
        "ORDER BY status DESC, updated_at DESC"));
    query.bindValue(QStringLiteral(":current_user_id"), currentUserId);
    query.bindValue(QStringLiteral(":target_user_id"), targetUserId);
    if (!query.exec()) {
        return 0;
    }

    int pendingStatus = 0;
    while (query.next()) {
        const auto userId = query.value(QStringLiteral("user_id")).toLongLong();
        const auto status = query.value(QStringLiteral("status")).toInt();
        if (status == 1) {
            return 1;
        }
        if (status == 0 && userId == currentUserId) {
            pendingStatus = 2;
        } else if (status == 0) {
            pendingStatus = 3;
        }
    }
    return pendingStatus;
}

bool userExists(QSqlDatabase db, qint64 userId)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT id FROM users WHERE id = :id AND is_deleted = 0 LIMIT 1"));
    query.bindValue(QStringLiteral(":id"), userId);
    return query.exec() && query.next();
}

} // namespace

bool FriendshipModel::searchUsers(qint64 currentUserId, const QString& keyword, QJsonArray* users)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, username, nickname, avatar, signature "
        "FROM users "
        "WHERE (username LIKE :keyword OR nickname LIKE :keyword) "
        "AND id != :current_user_id "
        "AND is_deleted = 0 "
        "LIMIT 20"));
    query.bindValue(QStringLiteral(":keyword"), QStringLiteral("%") + keyword + QStringLiteral("%"));
    query.bindValue(QStringLiteral(":current_user_id"), currentUserId);
    if (!query.exec()) {
        return false;
    }

    while (query.next()) {
        const auto targetUserId = query.value(QStringLiteral("id")).toLongLong();
        users->append(QJsonObject {
            { QStringLiteral("userId"), targetUserId },
            { QStringLiteral("username"), query.value(QStringLiteral("username")).toString() },
            { QStringLiteral("nickname"), query.value(QStringLiteral("nickname")).toString() },
            { QStringLiteral("avatar"), query.value(QStringLiteral("avatar")).toString() },
            { QStringLiteral("signature"), query.value(QStringLiteral("signature")).toString() },
            { QStringLiteral("friendStatus"), friendStatus(db, currentUserId, targetUserId) }
        });
    }
    return true;
}

bool FriendshipModel::apply(qint64 currentUserId, qint64 targetUserId, int* businessCode)
{
    *businessCode = 0;
    if (currentUserId == targetUserId) {
        *businessCode = 2004;
        return true;
    }

    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen() || !userExists(db, targetUserId)) {
        return false;
    }

    QSqlQuery relationQuery(db);
    relationQuery.prepare(QStringLiteral(
        "SELECT user_id, status FROM friendships "
        "WHERE ((user_id = :current_user_id AND friend_id = :target_user_id) "
        "OR (user_id = :target_user_id AND friend_id = :current_user_id)) "
        "AND is_deleted = 0"));
    relationQuery.bindValue(QStringLiteral(":current_user_id"), currentUserId);
    relationQuery.bindValue(QStringLiteral(":target_user_id"), targetUserId);
    if (!relationQuery.exec()) {
        return false;
    }

    bool hasRejectedForward = false;
    while (relationQuery.next()) {
        const auto userId = relationQuery.value(QStringLiteral("user_id")).toLongLong();
        const auto status = relationQuery.value(QStringLiteral("status")).toInt();
        if (status == 1) {
            *businessCode = 2002;
            return true;
        }
        if (status == 0 && userId == currentUserId) {
            *businessCode = 2003;
            return true;
        }
        if (status == 2 && userId == currentUserId) {
            hasRejectedForward = true;
        }
    }

    if (hasRejectedForward) {
        QSqlQuery updateQuery(db);
        updateQuery.prepare(QStringLiteral(
            "UPDATE friendships SET status = 0, is_deleted = 0 "
            "WHERE user_id = :current_user_id AND friend_id = :target_user_id"));
        updateQuery.bindValue(QStringLiteral(":current_user_id"), currentUserId);
        updateQuery.bindValue(QStringLiteral(":target_user_id"), targetUserId);
        return updateQuery.exec();
    }

    QSqlQuery insertQuery(db);
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO friendships (user_id, friend_id, status, created_at) "
        "VALUES (:current_user_id, :target_user_id, 0, NOW()) "
        "ON DUPLICATE KEY UPDATE status = 0, is_deleted = 0"));
    insertQuery.bindValue(QStringLiteral(":current_user_id"), currentUserId);
    insertQuery.bindValue(QStringLiteral(":target_user_id"), targetUserId);
    return insertQuery.exec();
}

bool FriendshipModel::fetchRequests(qint64 currentUserId, QJsonArray* requests)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT f.id, u.id AS from_user_id, u.username, u.nickname, u.avatar, %1 AS created_at "
        "FROM friendships f "
        "JOIN users u ON u.id = f.user_id "
        "WHERE f.friend_id = :current_user_id "
        "AND f.status = 0 "
        "AND f.is_deleted = 0 "
        "AND u.is_deleted = 0 "
        "ORDER BY f.created_at DESC").arg(formatDateTime(QStringLiteral("f.created_at"))));
    query.bindValue(QStringLiteral(":current_user_id"), currentUserId);
    if (!query.exec()) {
        return false;
    }

    while (query.next()) {
        requests->append(QJsonObject {
            { QStringLiteral("requestId"), query.value(QStringLiteral("id")).toLongLong() },
            { QStringLiteral("fromUserId"), query.value(QStringLiteral("from_user_id")).toLongLong() },
            { QStringLiteral("fromUsername"), query.value(QStringLiteral("username")).toString() },
            { QStringLiteral("fromNickname"), query.value(QStringLiteral("nickname")).toString() },
            { QStringLiteral("fromAvatar"), query.value(QStringLiteral("avatar")).toString() },
            { QStringLiteral("createdAt"), query.value(QStringLiteral("created_at")).toString() }
        });
    }
    return true;
}

bool FriendshipModel::handleRequest(qint64 currentUserId, qint64 requestId, int action)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen() || (action != 1 && action != 2)) {
        return false;
    }

    QSqlQuery selectQuery(db);
    selectQuery.prepare(QStringLiteral(
        "SELECT user_id FROM friendships "
        "WHERE id = :request_id AND friend_id = :current_user_id AND status = 0 AND is_deleted = 0 "
        "LIMIT 1"));
    selectQuery.bindValue(QStringLiteral(":request_id"), requestId);
    selectQuery.bindValue(QStringLiteral(":current_user_id"), currentUserId);
    if (!selectQuery.exec() || !selectQuery.next()) {
        return false;
    }
    const auto fromUserId = selectQuery.value(QStringLiteral("user_id")).toLongLong();

    if (!db.transaction()) {
        return false;
    }

    QSqlQuery updateQuery(db);
    updateQuery.prepare(QStringLiteral(
        "UPDATE friendships SET status = :status "
        "WHERE id = :request_id AND friend_id = :current_user_id"));
    updateQuery.bindValue(QStringLiteral(":status"), action);
    updateQuery.bindValue(QStringLiteral(":request_id"), requestId);
    updateQuery.bindValue(QStringLiteral(":current_user_id"), currentUserId);
    if (!updateQuery.exec()) {
        db.rollback();
        return false;
    }

    if (action == 1) {
        QSqlQuery reverseQuery(db);
        reverseQuery.prepare(QStringLiteral(
            "INSERT INTO friendships (user_id, friend_id, status) "
            "VALUES (:current_user_id, :from_user_id, 1) "
            "ON DUPLICATE KEY UPDATE status = 1, is_deleted = 0"));
        reverseQuery.bindValue(QStringLiteral(":current_user_id"), currentUserId);
        reverseQuery.bindValue(QStringLiteral(":from_user_id"), fromUserId);
        if (!reverseQuery.exec()) {
            db.rollback();
            return false;
        }
    }

    return db.commit();
}

bool FriendshipModel::fetchFriends(qint64 currentUserId, QJsonArray* friends)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT u.id, u.username, u.nickname, u.avatar, u.signature, u.status "
        "FROM friendships f "
        "JOIN users u ON u.id = f.friend_id "
        "WHERE f.user_id = :current_user_id "
        "AND f.status = 1 "
        "AND f.is_deleted = 0 "
        "AND u.is_deleted = 0 "
        "ORDER BY u.username ASC"));
    query.bindValue(QStringLiteral(":current_user_id"), currentUserId);
    if (!query.exec()) {
        return false;
    }

    while (query.next()) {
        friends->append(QJsonObject {
            { QStringLiteral("userId"), query.value(QStringLiteral("id")).toLongLong() },
            { QStringLiteral("username"), query.value(QStringLiteral("username")).toString() },
            { QStringLiteral("nickname"), query.value(QStringLiteral("nickname")).toString() },
            { QStringLiteral("avatar"), query.value(QStringLiteral("avatar")).toString() },
            { QStringLiteral("signature"), query.value(QStringLiteral("signature")).toString() },
            { QStringLiteral("isOnline"), query.value(QStringLiteral("status")).toInt() == 1 }
        });
    }
    return true;
}

bool FriendshipModel::ensureConversation(qint64 currentUserId, qint64 targetUserId, QJsonObject* conversation)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen() || currentUserId == targetUserId) {
        return false;
    }

    QSqlQuery insertQuery(db);
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO conversations (user_id, target_user_id) "
        "VALUES (:current_user_id, :target_user_id) "
        "ON DUPLICATE KEY UPDATE id = LAST_INSERT_ID(id), is_deleted = 0, updated_at = CURRENT_TIMESTAMP"));
    insertQuery.bindValue(QStringLiteral(":current_user_id"), currentUserId);
    insertQuery.bindValue(QStringLiteral(":target_user_id"), targetUserId);
    if (!insertQuery.exec()) {
        return false;
    }
    const auto conversationId = insertQuery.lastInsertId().toLongLong();

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, nickname, avatar, status FROM users "
        "WHERE id = :target_user_id AND is_deleted = 0 LIMIT 1"));
    query.bindValue(QStringLiteral(":target_user_id"), targetUserId);
    if (!query.exec() || !query.next()) {
        return false;
    }

    *conversation = QJsonObject {
        { QStringLiteral("conversationId"), conversationId },
        { QStringLiteral("targetUserId"), targetUserId },
        { QStringLiteral("targetNickname"), query.value(QStringLiteral("nickname")).toString() },
        { QStringLiteral("targetAvatar"), query.value(QStringLiteral("avatar")).toString() },
        { QStringLiteral("isOnline"), query.value(QStringLiteral("status")).toInt() == 1 }
    };
    return true;
}

} // namespace Backend::Models
