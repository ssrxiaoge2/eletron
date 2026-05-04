#include "MessageModel.h"

#include "../core/Database.h"
#include "../core/JwtHelper.h"

#include <algorithm>
#include <QList>
#include <QSqlQuery>
#include <QVariant>

namespace Backend::Models {
namespace {

constexpr auto DateTimeFormatSql = "DATE_FORMAT(%1, '%Y-%m-%d %H:%i:%s')";

QString formatExpression(const QString& column)
{
    return QString::fromLatin1(DateTimeFormatSql).arg(column);
}

} // namespace

bool MessageModel::findUserIdByToken(const QString& token, qint64* userId)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen() || token.isEmpty()) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT user_id FROM sessions "
        "WHERE token_hash = :token_hash "
        "AND expired_at > UTC_TIMESTAMP() "
        "AND is_deleted = 0 "
        "LIMIT 1"));
    query.bindValue(QStringLiteral(":token_hash"), Core::JwtHelper::sha256Hex(token));
    if (!query.exec() || !query.next()) {
        return false;
    }

    *userId = query.value(QStringLiteral("user_id")).toLongLong();
    return true;
}

bool MessageModel::areFriends(qint64 userId, qint64 targetUserId, bool* result)
{
    *result = false;
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id FROM friendships "
        "WHERE user_id = :user_id "
        "AND friend_id = :target_user_id "
        "AND status = 1 "
        "AND is_deleted = 0 "
        "LIMIT 1"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":target_user_id"), targetUserId);
    if (!query.exec()) {
        return false;
    }

    *result = query.next();
    return true;
}

bool MessageModel::fetchConversations(qint64 userId, QJsonArray* conversations)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }

    QSqlQuery targetsQuery(db);
    targetsQuery.prepare(QStringLiteral(
        "SELECT DISTINCT IF(sender_id = :user_id, receiver_id, sender_id) AS target_id "
        "FROM messages "
        "WHERE (sender_id = :user_id OR receiver_id = :user_id) "
        "AND is_deleted = 0"));
    targetsQuery.bindValue(QStringLiteral(":user_id"), userId);
    if (!targetsQuery.exec()) {
        return false;
    }

    struct ConversationRow {
        QJsonObject payload;
        QString lastMessageTime;
    };
    QList<ConversationRow> rows;

    while (targetsQuery.next()) {
        const auto targetId = targetsQuery.value(QStringLiteral("target_id")).toLongLong();

        QSqlQuery detailQuery(db);
        detailQuery.prepare(QStringLiteral(
            "SELECT u.id AS target_id, u.username, u.avatar, u.status, "
            "m.content, %1 AS last_message_time, "
            "(SELECT COUNT(*) FROM messages um "
            " WHERE um.sender_id = :target_id "
            " AND um.receiver_id = :user_id "
            " AND um.is_read = 0 "
            " AND um.is_deleted = 0) AS unread_count "
            "FROM users u "
            "JOIN messages m ON ((m.sender_id = :user_id AND m.receiver_id = u.id) "
            " OR (m.sender_id = u.id AND m.receiver_id = :user_id)) "
            "WHERE u.id = :target_id "
            "AND u.is_deleted = 0 "
            "AND m.is_deleted = 0 "
            "ORDER BY m.created_at DESC, m.id DESC "
            "LIMIT 1").arg(formatExpression(QStringLiteral("m.created_at"))));
        detailQuery.bindValue(QStringLiteral(":user_id"), userId);
        detailQuery.bindValue(QStringLiteral(":target_id"), targetId);
        if (!detailQuery.exec()) {
            return false;
        }
        if (!detailQuery.next()) {
            continue;
        }

        const auto lastMessageTime = detailQuery.value(QStringLiteral("last_message_time")).toString();
        rows.append({
            QJsonObject {
                { QStringLiteral("conversationId"), targetId },
                { QStringLiteral("targetUserId"), targetId },
                { QStringLiteral("targetUsername"), detailQuery.value(QStringLiteral("username")).toString() },
                { QStringLiteral("targetAvatar"), detailQuery.value(QStringLiteral("avatar")).toString() },
                { QStringLiteral("lastMessage"), detailQuery.value(QStringLiteral("content")).toString() },
                { QStringLiteral("lastMessageTime"), lastMessageTime },
                { QStringLiteral("unreadCount"), detailQuery.value(QStringLiteral("unread_count")).toInt() },
                { QStringLiteral("isOnline"), detailQuery.value(QStringLiteral("status")).toInt() == 1 }
            },
            lastMessageTime
        });
    }

    std::sort(rows.begin(), rows.end(), [](const ConversationRow& left, const ConversationRow& right) {
        return left.lastMessageTime > right.lastMessageTime;
    });

    for (const auto& row : rows) {
        conversations->append(row.payload);
    }
    return true;
}

bool MessageModel::fetchHistory(qint64 userId,
                                qint64 targetUserId,
                                int page,
                                int pageSize,
                                int* total,
                                QJsonArray* messages)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }

    QSqlQuery countQuery(db);
    countQuery.prepare(QStringLiteral(
        "SELECT COUNT(*) AS total FROM messages "
        "WHERE ((sender_id = :user_id AND receiver_id = :target_user_id) "
        "OR (sender_id = :target_user_id AND receiver_id = :user_id)) "
        "AND is_deleted = 0"));
    countQuery.bindValue(QStringLiteral(":user_id"), userId);
    countQuery.bindValue(QStringLiteral(":target_user_id"), targetUserId);
    if (!countQuery.exec() || !countQuery.next()) {
        return false;
    }
    *total = countQuery.value(QStringLiteral("total")).toInt();

    const auto offset = (page - 1) * pageSize;
    QSqlQuery listQuery(db);
    listQuery.prepare(QStringLiteral(
        "SELECT id, sender_id, receiver_id, content, type, %1 AS created_at "
        "FROM messages "
        "WHERE ((sender_id = :user_id AND receiver_id = :target_user_id) "
        "OR (sender_id = :target_user_id AND receiver_id = :user_id)) "
        "AND is_deleted = 0 "
        "ORDER BY created_at ASC, id ASC "
        "LIMIT :limit OFFSET :offset").arg(formatExpression(QStringLiteral("created_at"))));
    listQuery.bindValue(QStringLiteral(":user_id"), userId);
    listQuery.bindValue(QStringLiteral(":target_user_id"), targetUserId);
    listQuery.bindValue(QStringLiteral(":limit"), pageSize);
    listQuery.bindValue(QStringLiteral(":offset"), offset);
    if (!listQuery.exec()) {
        return false;
    }

    while (listQuery.next()) {
        const auto senderId = listQuery.value(QStringLiteral("sender_id")).toLongLong();
        messages->append(QJsonObject {
            { QStringLiteral("messageId"), listQuery.value(QStringLiteral("id")).toLongLong() },
            { QStringLiteral("senderId"), senderId },
            { QStringLiteral("receiverId"), listQuery.value(QStringLiteral("receiver_id")).toLongLong() },
            { QStringLiteral("content"), listQuery.value(QStringLiteral("content")).toString() },
            { QStringLiteral("type"), listQuery.value(QStringLiteral("type")).toInt() },
            { QStringLiteral("createdAt"), listQuery.value(QStringLiteral("created_at")).toString() },
            { QStringLiteral("isSelf"), senderId == userId }
        });
    }

    return true;
}

bool MessageModel::insertMessage(qint64 senderId,
                                 qint64 receiverId,
                                 const QString& content,
                                 int type,
                                 qint64* messageId,
                                 QString* createdAt)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }

    QSqlQuery insertQuery(db);
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO messages (sender_id, receiver_id, content, type) "
        "VALUES (:sender_id, :receiver_id, :content, :type)"));
    insertQuery.bindValue(QStringLiteral(":sender_id"), senderId);
    insertQuery.bindValue(QStringLiteral(":receiver_id"), receiverId);
    insertQuery.bindValue(QStringLiteral(":content"), content);
    insertQuery.bindValue(QStringLiteral(":type"), type);
    if (!insertQuery.exec()) {
        return false;
    }

    *messageId = insertQuery.lastInsertId().toLongLong();

    QSqlQuery selectQuery(db);
    selectQuery.prepare(QStringLiteral(
        "SELECT %1 AS created_at FROM messages WHERE id = :id LIMIT 1")
                            .arg(formatExpression(QStringLiteral("created_at"))));
    selectQuery.bindValue(QStringLiteral(":id"), *messageId);
    if (!selectQuery.exec() || !selectQuery.next()) {
        return false;
    }

    *createdAt = selectQuery.value(QStringLiteral("created_at")).toString();
    return true;
}

bool MessageModel::markConversationRead(qint64 userId, qint64 targetUserId, int* readCount)
{
    *readCount = 0;
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "UPDATE messages "
        "SET is_read = 1 "
        "WHERE sender_id = :target_user_id "
        "AND receiver_id = :user_id "
        "AND is_read = 0 "
        "AND is_deleted = 0"));
    query.bindValue(QStringLiteral(":target_user_id"), targetUserId);
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec()) {
        return false;
    }

    *readCount = query.numRowsAffected();
    return true;
}

} // namespace Backend::Models
