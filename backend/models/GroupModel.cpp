#include "GroupModel.h"

#include "../core/Database.h"

#include <QSet>
#include <QSqlQuery>
#include <QVariant>

namespace Backend::Models {
namespace {

QString dt(const QString& column)
{
    return QStringLiteral("DATE_FORMAT(%1, '%Y-%m-%d %H:%i:%s')").arg(column);
}

QString d(const QString& column)
{
    return QStringLiteral("DATE_FORMAT(%1, '%Y-%m-%d')").arg(column);
}

bool updateMemberCount(QSqlDatabase db, qint64 groupId)
{
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "UPDATE `groups` SET member_count = ("
        "SELECT COUNT(*) FROM group_members WHERE group_id = :group_id AND is_deleted = 0"
        ") WHERE id = :group_id"));
    query.bindValue(QStringLiteral(":group_id"), groupId);
    return query.exec();
}

} // namespace

bool GroupModel::create(qint64 ownerId,
                        const QString& name,
                        const QList<qint64>& memberIds,
                        qint64* groupId,
                        QString* createdAt)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen() || !db.transaction()) {
        return false;
    }

    const auto memberCount = memberIds.size() + 1;
    QSqlQuery groupQuery(db);
    groupQuery.prepare(QStringLiteral(
        "INSERT INTO `groups` (name, owner_id, member_count) "
        "VALUES (:name, :owner_id, :member_count)"));
    groupQuery.bindValue(QStringLiteral(":name"), name);
    groupQuery.bindValue(QStringLiteral(":owner_id"), ownerId);
    groupQuery.bindValue(QStringLiteral(":member_count"), memberCount);
    if (!groupQuery.exec()) {
        db.rollback();
        return false;
    }
    *groupId = groupQuery.lastInsertId().toLongLong();

    QSqlQuery memberQuery(db);
    memberQuery.prepare(QStringLiteral(
        "INSERT INTO group_members (group_id, user_id, role) "
        "VALUES (:group_id, :user_id, :role)"));
    memberQuery.bindValue(QStringLiteral(":group_id"), *groupId);
    memberQuery.bindValue(QStringLiteral(":user_id"), ownerId);
    memberQuery.bindValue(QStringLiteral(":role"), 2);
    if (!memberQuery.exec()) {
        db.rollback();
        return false;
    }

    for (const auto memberId : memberIds) {
        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "INSERT INTO group_members (group_id, user_id, role) "
            "VALUES (:group_id, :user_id, 0)"));
        query.bindValue(QStringLiteral(":group_id"), *groupId);
        query.bindValue(QStringLiteral(":user_id"), memberId);
        if (!query.exec()) {
            db.rollback();
            return false;
        }
    }

    QSqlQuery selectQuery(db);
    selectQuery.prepare(QStringLiteral("SELECT %1 AS created_at FROM `groups` WHERE id = :id").arg(dt(QStringLiteral("created_at"))));
    selectQuery.bindValue(QStringLiteral(":id"), *groupId);
    if (!selectQuery.exec() || !selectQuery.next()) {
        db.rollback();
        return false;
    }
    *createdAt = selectQuery.value(QStringLiteral("created_at")).toString();

    if (!db.commit()) {
        db.rollback();
        return false;
    }
    return true;
}

bool GroupModel::role(qint64 groupId, qint64 userId, int* roleValue)
{
    QSqlQuery query(Core::Database::getConnection());
    query.prepare(QStringLiteral(
        "SELECT role FROM group_members gm "
        "JOIN `groups` g ON g.id = gm.group_id "
        "WHERE gm.group_id = :group_id AND gm.user_id = :user_id "
        "AND gm.is_deleted = 0 AND g.is_dissolved = 0 LIMIT 1"));
    query.bindValue(QStringLiteral(":group_id"), groupId);
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec() || !query.next()) {
        return false;
    }
    *roleValue = query.value(QStringLiteral("role")).toInt();
    return true;
}

bool GroupModel::isActiveMember(qint64 groupId, qint64 userId, bool* result)
{
    *result = false;
    int r = 0;
    if (!role(groupId, userId, &r)) {
        return false;
    }
    *result = true;
    return true;
}

bool GroupModel::dissolve(qint64 groupId)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen() || !db.transaction()) {
        return false;
    }
    QSqlQuery groupQuery(db);
    groupQuery.prepare(QStringLiteral("UPDATE `groups` SET is_dissolved = 1 WHERE id = :group_id"));
    groupQuery.bindValue(QStringLiteral(":group_id"), groupId);
    if (!groupQuery.exec()) {
        db.rollback();
        return false;
    }
    QSqlQuery memberQuery(db);
    memberQuery.prepare(QStringLiteral("UPDATE group_members SET is_deleted = 1 WHERE group_id = :group_id"));
    memberQuery.bindValue(QStringLiteral(":group_id"), groupId);
    if (!memberQuery.exec()) {
        db.rollback();
        return false;
    }
    return db.commit();
}

bool GroupModel::updateName(qint64 groupId, const QString& name)
{
    QSqlQuery query(Core::Database::getConnection());
    query.prepare(QStringLiteral("UPDATE `groups` SET name = :name WHERE id = :group_id AND is_dissolved = 0"));
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":group_id"), groupId);
    return query.exec();
}

bool GroupModel::updateAnnouncement(qint64 groupId, const QString& announcement)
{
    QSqlQuery query(Core::Database::getConnection());
    query.prepare(QStringLiteral("UPDATE `groups` SET announcement = :announcement WHERE id = :group_id AND is_dissolved = 0"));
    query.bindValue(QStringLiteral(":announcement"), announcement);
    query.bindValue(QStringLiteral(":group_id"), groupId);
    return query.exec();
}

bool GroupModel::info(qint64 groupId, QJsonObject* out)
{
    QSqlQuery query(Core::Database::getConnection());
    query.prepare(QStringLiteral(
        "SELECT id, name, owner_id, announcement, member_count, max_member, avatar, %1 AS created_at "
        "FROM `groups` WHERE id = :group_id AND is_dissolved = 0 LIMIT 1").arg(d(QStringLiteral("created_at"))));
    query.bindValue(QStringLiteral(":group_id"), groupId);
    if (!query.exec() || !query.next()) {
        return false;
    }
    *out = QJsonObject {
        { QStringLiteral("groupId"), query.value(QStringLiteral("id")).toLongLong() },
        { QStringLiteral("name"), query.value(QStringLiteral("name")).toString() },
        { QStringLiteral("ownerId"), query.value(QStringLiteral("owner_id")).toLongLong() },
        { QStringLiteral("announcement"), query.value(QStringLiteral("announcement")).toString() },
        { QStringLiteral("memberCount"), query.value(QStringLiteral("member_count")).toInt() },
        { QStringLiteral("maxMember"), query.value(QStringLiteral("max_member")).toInt() },
        { QStringLiteral("avatar"), query.value(QStringLiteral("avatar")).toString() },
        { QStringLiteral("createdAt"), query.value(QStringLiteral("created_at")).toString() }
    };
    return true;
}

bool GroupModel::members(qint64 groupId, QJsonArray* out)
{
    QSqlQuery query(Core::Database::getConnection());
    query.prepare(QStringLiteral(
        "SELECT u.id, u.username, u.nickname, u.avatar, gm.role, gm.is_muted, "
        "%1 AS mute_until, %2 AS joined_at "
        "FROM group_members gm JOIN users u ON u.id = gm.user_id "
        "WHERE gm.group_id = :group_id AND gm.is_deleted = 0 AND u.is_deleted = 0 "
        "ORDER BY gm.role DESC, gm.joined_at ASC").arg(dt(QStringLiteral("gm.mute_until")), d(QStringLiteral("gm.joined_at"))));
    query.bindValue(QStringLiteral(":group_id"), groupId);
    if (!query.exec()) {
        return false;
    }
    while (query.next()) {
        const auto muteUntil = query.value(QStringLiteral("mute_until")).toString();
        out->append(QJsonObject {
            { QStringLiteral("userId"), query.value(QStringLiteral("id")).toLongLong() },
            { QStringLiteral("username"), query.value(QStringLiteral("username")).toString() },
            { QStringLiteral("nickname"), query.value(QStringLiteral("nickname")).toString() },
            { QStringLiteral("avatar"), query.value(QStringLiteral("avatar")).toString() },
            { QStringLiteral("role"), query.value(QStringLiteral("role")).toInt() },
            { QStringLiteral("isMuted"), query.value(QStringLiteral("is_muted")).toInt() == 1 },
            { QStringLiteral("muteUntil"), muteUntil.isEmpty() ? QJsonValue() : QJsonValue(muteUntil) },
            { QStringLiteral("joinedAt"), query.value(QStringLiteral("joined_at")).toString() }
        });
    }
    return true;
}

bool GroupModel::setRole(qint64 groupId, qint64 userId, int roleValue)
{
    QSqlQuery query(Core::Database::getConnection());
    query.prepare(QStringLiteral(
        "UPDATE group_members SET role = :role "
        "WHERE group_id = :group_id AND user_id = :user_id AND role <> 2 AND is_deleted = 0"));
    query.bindValue(QStringLiteral(":role"), roleValue);
    query.bindValue(QStringLiteral(":group_id"), groupId);
    query.bindValue(QStringLiteral(":user_id"), userId);
    return query.exec();
}

bool GroupModel::memberRole(qint64 groupId, qint64 userId, int* roleValue)
{
    return role(groupId, userId, roleValue);
}

bool GroupModel::removeMember(qint64 groupId, qint64 userId)
{
    auto db = Core::Database::getConnection();
    if (!db.isValid() || !db.isOpen() || !db.transaction()) {
        return false;
    }
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "UPDATE group_members SET is_deleted = 1 WHERE group_id = :group_id AND user_id = :user_id AND is_deleted = 0"));
    query.bindValue(QStringLiteral(":group_id"), groupId);
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec() || !updateMemberCount(db, groupId)) {
        db.rollback();
        return false;
    }
    return db.commit();
}

bool GroupModel::muteMember(qint64 groupId, qint64 userId, int duration)
{
    QSqlQuery query(Core::Database::getConnection());
    if (duration == 0) {
        query.prepare(QStringLiteral(
            "UPDATE group_members SET is_muted = 0, mute_until = NULL "
            "WHERE group_id = :group_id AND user_id = :user_id AND is_deleted = 0"));
    } else {
        query.prepare(QStringLiteral(
            "UPDATE group_members SET is_muted = 1, mute_until = DATE_ADD(NOW(), INTERVAL :duration SECOND) "
            "WHERE group_id = :group_id AND user_id = :user_id AND is_deleted = 0"));
        query.bindValue(QStringLiteral(":duration"), duration);
    }
    query.bindValue(QStringLiteral(":group_id"), groupId);
    query.bindValue(QStringLiteral(":user_id"), userId);
    return query.exec();
}

bool GroupModel::listMine(qint64 userId, QJsonArray* out)
{
    QSqlQuery query(Core::Database::getConnection());
    query.prepare(QStringLiteral(
        "SELECT g.id, g.name, g.avatar, g.member_count, gm.role AS my_role, "
        "(SELECT content FROM group_messages m WHERE m.group_id = g.id AND m.is_deleted = 0 ORDER BY m.created_at DESC, m.id DESC LIMIT 1) AS last_message, "
        "(SELECT type FROM group_messages m WHERE m.group_id = g.id AND m.is_deleted = 0 ORDER BY m.created_at DESC, m.id DESC LIMIT 1) AS last_message_type, "
        "(SELECT %1 FROM group_messages m WHERE m.group_id = g.id AND m.is_deleted = 0 ORDER BY m.created_at DESC, m.id DESC LIMIT 1) AS last_message_time "
        "FROM group_members gm JOIN `groups` g ON g.id = gm.group_id "
        "WHERE gm.user_id = :user_id AND gm.is_deleted = 0 AND g.is_dissolved = 0 "
        "ORDER BY last_message_time DESC, g.updated_at DESC").arg(dt(QStringLiteral("m.created_at"))));
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec()) {
        return false;
    }
    while (query.next()) {
        out->append(QJsonObject {
            { QStringLiteral("groupId"), query.value(QStringLiteral("id")).toLongLong() },
            { QStringLiteral("name"), query.value(QStringLiteral("name")).toString() },
            { QStringLiteral("avatar"), query.value(QStringLiteral("avatar")).toString() },
            { QStringLiteral("lastMessage"), query.value(QStringLiteral("last_message")).toString() },
            { QStringLiteral("lastMessageType"), query.value(QStringLiteral("last_message_type")).isNull() ? 0 : query.value(QStringLiteral("last_message_type")).toInt() },
            { QStringLiteral("lastMessageTime"), query.value(QStringLiteral("last_message_time")).toString() },
            { QStringLiteral("unreadCount"), 0 },
            { QStringLiteral("myRole"), query.value(QStringLiteral("my_role")).toInt() },
            { QStringLiteral("memberCount"), query.value(QStringLiteral("member_count")).toInt() }
        });
    }
    return true;
}

} // namespace Backend::Models
