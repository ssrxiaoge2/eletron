#include "FriendGroupModel.h"

#include "../core/Database.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

namespace Backend::Models {
namespace {

constexpr auto kDefaultGroupName = "我的好友";

bool groupBelongsToUser(QSqlDatabase db, qint64 userId, qint64 groupId, QString* name = nullptr)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT name FROM friend_groups "
        "WHERE id = :group_id AND user_id = :user_id "
        "LIMIT 1"));
    query.bindValue(QStringLiteral(":group_id"), groupId);
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec() || !query.next()) {
        return false;
    }
    if (name) {
        *name = query.value(QStringLiteral("name")).toString();
    }
    return true;
}

bool nameExists(QSqlDatabase db, qint64 userId, const QString& name, qint64 exceptGroupId = 0)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id FROM friend_groups "
        "WHERE user_id = :user_id AND name = :name "
        "AND (:except_group_id = 0 OR id != :except_group_id) "
        "LIMIT 1"));
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":except_group_id"), exceptGroupId);
    return query.exec() && query.next();
}

QJsonObject friendJson(const QSqlQuery& query)
{
    return QJsonObject {
        { QStringLiteral("userId"), query.value(QStringLiteral("friend_user_id")).toLongLong() },
        { QStringLiteral("username"), query.value(QStringLiteral("username")).toString() },
        { QStringLiteral("nickname"), query.value(QStringLiteral("nickname")).toString() },
        { QStringLiteral("avatar"), query.value(QStringLiteral("avatar")).toString() },
        { QStringLiteral("signature"), query.value(QStringLiteral("signature")).toString() },
        { QStringLiteral("isOnline"), query.value(QStringLiteral("status")).toInt() == 1 },
        { QStringLiteral("friendGroupId"), query.value(QStringLiteral("friend_group_id")).toLongLong() }
    };
}

} // namespace

bool FriendGroupModel::ensureDefaultGroup(qint64 userId, qint64* groupId)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }

    QSqlQuery insertQuery(db);
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO friend_groups (user_id, name, sort_order) "
        "VALUES (:user_id, :name, 0) "
        "ON DUPLICATE KEY UPDATE id = LAST_INSERT_ID(id)"));
    insertQuery.bindValue(QStringLiteral(":user_id"), userId);
    insertQuery.bindValue(QStringLiteral(":name"), QString::fromUtf8(kDefaultGroupName));
    if (!insertQuery.exec()) {
        return false;
    }

    if (groupId) {
        const auto insertedId = insertQuery.lastInsertId().toLongLong();
        if (insertedId > 0) {
            *groupId = insertedId;
            return true;
        }

        QSqlQuery selectQuery(db);
        selectQuery.prepare(QStringLiteral(
            "SELECT id FROM friend_groups WHERE user_id = :user_id AND name = :name LIMIT 1"));
        selectQuery.bindValue(QStringLiteral(":user_id"), userId);
        selectQuery.bindValue(QStringLiteral(":name"), QString::fromUtf8(kDefaultGroupName));
        if (!selectQuery.exec() || !selectQuery.next()) {
            return false;
        }
        *groupId = selectQuery.value(QStringLiteral("id")).toLongLong();
    }
    return true;
}

bool FriendGroupModel::listWithFriends(qint64 userId, QJsonArray* groups)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }
    if (!ensureDefaultGroup(userId)) {
        return false;
    }

    QSqlQuery groupQuery(db);
    groupQuery.prepare(QStringLiteral(
        "SELECT id, name, sort_order FROM friend_groups "
        "WHERE user_id = :user_id "
        "ORDER BY sort_order ASC, id ASC"));
    groupQuery.bindValue(QStringLiteral(":user_id"), userId);
    if (!groupQuery.exec()) {
        return false;
    }

    while (groupQuery.next()) {
        const auto groupId = groupQuery.value(QStringLiteral("id")).toLongLong();
        QSqlQuery friendQuery(db);
        friendQuery.prepare(QStringLiteral(
            "SELECT u.id AS friend_user_id, u.username, u.nickname, u.avatar, u.signature, u.status, "
            "COALESCE(f.group_id, :group_id) AS friend_group_id "
            "FROM friendships f "
            "JOIN users u ON u.id = f.friend_id "
            "WHERE f.user_id = :user_id "
            "AND f.status = 1 "
            "AND f.is_deleted = 0 "
            "AND u.is_deleted = 0 "
            "AND (f.group_id = :group_id OR (f.group_id IS NULL AND :is_default = 1)) "
            "ORDER BY u.status DESC, u.nickname ASC, u.username ASC"));
        friendQuery.bindValue(QStringLiteral(":user_id"), userId);
        friendQuery.bindValue(QStringLiteral(":group_id"), groupId);
        friendQuery.bindValue(QStringLiteral(":is_default"),
                              groupQuery.value(QStringLiteral("name")).toString() == QString::fromUtf8(kDefaultGroupName) ? 1 : 0);
        if (!friendQuery.exec()) {
            return false;
        }

        QJsonArray friends;
        int onlineCount = 0;
        while (friendQuery.next()) {
            if (friendQuery.value(QStringLiteral("status")).toInt() == 1) {
                ++onlineCount;
            }
            friends.append(friendJson(friendQuery));
        }

        groups->append(QJsonObject {
            { QStringLiteral("groupId"), groupId },
            { QStringLiteral("name"), groupQuery.value(QStringLiteral("name")).toString() },
            { QStringLiteral("sortOrder"), groupQuery.value(QStringLiteral("sort_order")).toInt() },
            { QStringLiteral("onlineCount"), onlineCount },
            { QStringLiteral("totalCount"), friends.size() },
            { QStringLiteral("friends"), friends }
        });
    }
    return true;
}

bool FriendGroupModel::create(qint64 userId, const QString& name, QJsonObject* group, int* businessCode)
{
    *businessCode = 0;
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }
    if (nameExists(db, userId, name)) {
        *businessCode = 6001;
        return true;
    }

    QSqlQuery maxQuery(db);
    maxQuery.prepare(QStringLiteral(
        "SELECT COALESCE(MAX(sort_order), -1) + 1 AS next_sort_order "
        "FROM friend_groups WHERE user_id = :user_id"));
    maxQuery.bindValue(QStringLiteral(":user_id"), userId);
    if (!maxQuery.exec() || !maxQuery.next()) {
        return false;
    }
    const auto sortOrder = maxQuery.value(QStringLiteral("next_sort_order")).toInt();

    QSqlQuery insertQuery(db);
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO friend_groups (user_id, name, sort_order) "
        "VALUES (:user_id, :name, :sort_order)"));
    insertQuery.bindValue(QStringLiteral(":user_id"), userId);
    insertQuery.bindValue(QStringLiteral(":name"), name);
    insertQuery.bindValue(QStringLiteral(":sort_order"), sortOrder);
    if (!insertQuery.exec()) {
        return false;
    }

    *group = QJsonObject {
        { QStringLiteral("groupId"), insertQuery.lastInsertId().toLongLong() },
        { QStringLiteral("name"), name },
        { QStringLiteral("sortOrder"), sortOrder }
    };
    return true;
}

bool FriendGroupModel::rename(qint64 userId, qint64 groupId, const QString& name, int* businessCode)
{
    *businessCode = 0;
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }
    if (!groupBelongsToUser(db, userId, groupId)) {
        *businessCode = 6003;
        return true;
    }
    if (nameExists(db, userId, name, groupId)) {
        *businessCode = 6001;
        return true;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "UPDATE friend_groups SET name = :name WHERE id = :group_id AND user_id = :user_id"));
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":group_id"), groupId);
    query.bindValue(QStringLiteral(":user_id"), userId);
    return query.exec();
}

bool FriendGroupModel::remove(qint64 userId, qint64 groupId, int* businessCode)
{
    *businessCode = 0;
    qint64 defaultGroupId = 0;
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen() || !ensureDefaultGroup(userId, &defaultGroupId)) {
        return false;
    }

    QString groupName;
    if (!groupBelongsToUser(db, userId, groupId, &groupName)) {
        *businessCode = 6003;
        return true;
    }
    if (groupName == QString::fromUtf8(kDefaultGroupName)) {
        *businessCode = 6002;
        return true;
    }

    if (!db.transaction()) {
        return false;
    }

    QSqlQuery moveQuery(db);
    moveQuery.prepare(QStringLiteral(
        "UPDATE friendships SET group_id = :default_group_id "
        "WHERE user_id = :user_id AND group_id = :group_id"));
    moveQuery.bindValue(QStringLiteral(":default_group_id"), defaultGroupId);
    moveQuery.bindValue(QStringLiteral(":user_id"), userId);
    moveQuery.bindValue(QStringLiteral(":group_id"), groupId);
    if (!moveQuery.exec()) {
        db.rollback();
        return false;
    }

    QSqlQuery deleteQuery(db);
    deleteQuery.prepare(QStringLiteral(
        "DELETE FROM friend_groups WHERE id = :group_id AND user_id = :user_id"));
    deleteQuery.bindValue(QStringLiteral(":group_id"), groupId);
    deleteQuery.bindValue(QStringLiteral(":user_id"), userId);
    if (!deleteQuery.exec()) {
        db.rollback();
        return false;
    }
    return db.commit();
}

bool FriendGroupModel::moveFriend(qint64 userId, qint64 friendUserId, qint64 groupId, int* businessCode)
{
    *businessCode = 0;
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }
    if (!groupBelongsToUser(db, userId, groupId)) {
        *businessCode = 6003;
        return true;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "UPDATE friendships SET group_id = :group_id "
        "WHERE user_id = :user_id "
        "AND friend_id = :friend_user_id "
        "AND status = 1 "
        "AND is_deleted = 0"));
    query.bindValue(QStringLiteral(":group_id"), groupId);
    query.bindValue(QStringLiteral(":user_id"), userId);
    query.bindValue(QStringLiteral(":friend_user_id"), friendUserId);
    if (!query.exec()) {
        return false;
    }
    if (query.numRowsAffected() <= 0) {
        *businessCode = 6003;
    }
    return true;
}

} // namespace Backend::Models
