#include "GroupMessageModel.h"

#include "../core/Database.h"

#include <QJsonObject>
#include <QSqlQuery>
#include <QVariant>

namespace Backend::Models {
namespace {

QString dt(const QString& column)
{
    return QStringLiteral("DATE_FORMAT(%1, '%Y-%m-%d %H:%i:%s')").arg(column);
}

} // namespace

bool GroupMessageModel::muteState(qint64 groupId, qint64 userId, GroupMuteState* state)
{
    QSqlQuery query(Core::Database::getConnection());
    query.prepare(QStringLiteral(
        "SELECT is_muted, %1 AS mute_until FROM group_members "
        "WHERE group_id = :group_id AND user_id = :user_id AND is_deleted = 0 LIMIT 1")
                      .arg(dt(QStringLiteral("mute_until"))));
    query.bindValue(QStringLiteral(":group_id"), groupId);
    query.bindValue(QStringLiteral(":user_id"), userId);
    if (!query.exec() || !query.next()) {
        return false;
    }
    state->muted = query.value(QStringLiteral("is_muted")).toInt() == 1
        && !query.value(QStringLiteral("mute_until")).toString().isEmpty();
    state->muteUntil = query.value(QStringLiteral("mute_until")).toString();
    if (state->muted) {
        QSqlQuery activeQuery(Core::Database::getConnection());
        activeQuery.prepare(QStringLiteral(
            "SELECT 1 FROM group_members "
            "WHERE group_id = :group_id AND user_id = :user_id "
            "AND is_muted = 1 AND mute_until > NOW() AND is_deleted = 0 LIMIT 1"));
        activeQuery.bindValue(QStringLiteral(":group_id"), groupId);
        activeQuery.bindValue(QStringLiteral(":user_id"), userId);
        if (!activeQuery.exec()) {
            return false;
        }
        state->muted = activeQuery.next();
    }
    return true;
}

bool GroupMessageModel::insert(qint64 groupId,
                               qint64 senderId,
                               const QString& content,
                               int type,
                               qint64* messageId,
                               QString* createdAt)
{
    QSqlQuery insertQuery(Core::Database::getConnection());
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO group_messages (group_id, sender_id, content, type) "
        "VALUES (:group_id, :sender_id, :content, :type)"));
    insertQuery.bindValue(QStringLiteral(":group_id"), groupId);
    insertQuery.bindValue(QStringLiteral(":sender_id"), senderId);
    insertQuery.bindValue(QStringLiteral(":content"), content);
    insertQuery.bindValue(QStringLiteral(":type"), type);
    if (!insertQuery.exec()) {
        return false;
    }
    *messageId = insertQuery.lastInsertId().toLongLong();

    QSqlQuery selectQuery(Core::Database::getConnection());
    selectQuery.prepare(QStringLiteral(
        "SELECT %1 AS created_at FROM group_messages WHERE id = :id LIMIT 1")
                            .arg(dt(QStringLiteral("created_at"))));
    selectQuery.bindValue(QStringLiteral(":id"), *messageId);
    if (!selectQuery.exec() || !selectQuery.next()) {
        return false;
    }
    *createdAt = selectQuery.value(QStringLiteral("created_at")).toString();
    return true;
}

bool GroupMessageModel::history(qint64 groupId,
                                qint64 currentUserId,
                                int page,
                                int pageSize,
                                int* total,
                                QJsonArray* messages)
{
    QSqlQuery countQuery(Core::Database::getConnection());
    countQuery.prepare(QStringLiteral(
        "SELECT COUNT(*) AS total FROM group_messages "
        "WHERE group_id = :group_id AND is_deleted = 0"));
    countQuery.bindValue(QStringLiteral(":group_id"), groupId);
    if (!countQuery.exec() || !countQuery.next()) {
        return false;
    }
    *total = countQuery.value(QStringLiteral("total")).toInt();

    QSqlQuery query(Core::Database::getConnection());
    query.prepare(QStringLiteral(
        "SELECT m.id, m.sender_id, u.nickname, u.avatar, gm.role, m.content, m.type, %1 AS created_at "
        "FROM group_messages m "
        "JOIN users u ON u.id = m.sender_id "
        "LEFT JOIN group_members gm ON gm.group_id = m.group_id AND gm.user_id = m.sender_id "
        "WHERE m.group_id = :group_id AND m.is_deleted = 0 "
        "ORDER BY m.created_at ASC, m.id ASC LIMIT :limit OFFSET :offset")
                      .arg(dt(QStringLiteral("m.created_at"))));
    query.bindValue(QStringLiteral(":group_id"), groupId);
    query.bindValue(QStringLiteral(":limit"), pageSize);
    query.bindValue(QStringLiteral(":offset"), (page - 1) * pageSize);
    if (!query.exec()) {
        return false;
    }
    while (query.next()) {
        const auto senderId = query.value(QStringLiteral("sender_id")).toLongLong();
        messages->append(QJsonObject {
            { QStringLiteral("messageId"), query.value(QStringLiteral("id")).toLongLong() },
            { QStringLiteral("senderId"), senderId },
            { QStringLiteral("senderNickname"), query.value(QStringLiteral("nickname")).toString() },
            { QStringLiteral("senderAvatar"), query.value(QStringLiteral("avatar")).toString() },
            { QStringLiteral("senderRole"), query.value(QStringLiteral("role")).toInt() },
            { QStringLiteral("content"), query.value(QStringLiteral("content")).toString() },
            { QStringLiteral("type"), query.value(QStringLiteral("type")).toInt() },
            { QStringLiteral("createdAt"), query.value(QStringLiteral("created_at")).toString() },
            { QStringLiteral("isSelf"), senderId == currentUserId }
        });
    }
    return true;
}

} // namespace Backend::Models
