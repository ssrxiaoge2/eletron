#include "GroupService.h"

#include "../models/GroupMessageModel.h"
#include "../models/GroupModel.h"
#include "../models/MessageModel.h"

#include <QJsonArray>
#include <QSet>

namespace Backend::Services {
namespace {

QString tokenFromBearer(const QString& bearerToken)
{
    const auto trimmed = bearerToken.trimmed();
    if (!trimmed.startsWith(QStringLiteral("Bearer "), Qt::CaseInsensitive)) {
        return {};
    }
    return trimmed.mid(QStringLiteral("Bearer ").size()).trimmed();
}

bool memberRole(qint64 groupId, qint64 userId, int* role)
{
    return Models::GroupModel::role(groupId, userId, role);
}

bool activeMember(qint64 groupId, qint64 userId)
{
    bool result = false;
    return Models::GroupModel::isActiveMember(groupId, userId, &result) && result;
}

} // namespace

GroupResult GroupService::create(const QString& bearerToken, const QJsonObject& body) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }

    const auto name = body.value(QStringLiteral("name")).toString().trimmed();
    const auto memberArray = body.value(QStringLiteral("memberIds")).toArray();
    if (name.isEmpty() || name.size() > 64 || memberArray.size() < 1) {
        return error(400, 1000, QStringLiteral("invalid group payload"));
    }
    if (memberArray.size() > 99) {
        return error(400, 5002, QStringLiteral("group member limit exceeded"));
    }

    QList<qint64> memberIds;
    QSet<qint64> dedup;
    for (const auto& value : memberArray) {
        const auto memberId = value.toVariant().toLongLong();
        if (memberId <= 0 || memberId == userId || dedup.contains(memberId)) {
            continue;
        }
        bool areFriends = false;
        if (!Models::MessageModel::areFriends(userId, memberId, &areFriends)) {
            return error(503, 2002, QStringLiteral("database_error"));
        }
        if (!areFriends) {
            return error(403, 5001, QStringLiteral("target user is not your friend"));
        }
        dedup.insert(memberId);
        memberIds.append(memberId);
    }
    if (memberIds.isEmpty()) {
        return error(400, 1000, QStringLiteral("invalid group payload"));
    }

    qint64 groupId = 0;
    QString createdAt;
    if (!Models::GroupModel::create(userId, name, memberIds, &groupId, &createdAt)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }

    GroupResult result;
    result.data = QJsonObject {
        { QStringLiteral("groupId"), groupId },
        { QStringLiteral("name"), name },
        { QStringLiteral("memberCount"), memberIds.size() + 1 },
        { QStringLiteral("createdAt"), createdAt }
    };
    return result;
}

GroupResult GroupService::dissolve(const QString& bearerToken, qint64 groupId) const
{
    qint64 userId = 0;
    int role = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (!memberRole(groupId, userId, &role) || role != 2) {
        return error(403, 5003, QStringLiteral("permission denied"));
    }
    if (!Models::GroupModel::dissolve(groupId)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }
    return {};
}

GroupResult GroupService::updateName(const QString& bearerToken, qint64 groupId, const QJsonObject& body) const
{
    qint64 userId = 0;
    int role = 0;
    const auto name = body.value(QStringLiteral("name")).toString().trimmed();
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (name.isEmpty() || name.size() > 64) {
        return error(400, 1000, QStringLiteral("invalid group name"));
    }
    if (!memberRole(groupId, userId, &role) || role != 2) {
        return error(403, 5003, QStringLiteral("permission denied"));
    }
    if (!Models::GroupModel::updateName(groupId, name)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }
    return {};
}

GroupResult GroupService::updateAnnouncement(const QString& bearerToken, qint64 groupId, const QJsonObject& body) const
{
    qint64 userId = 0;
    int role = 0;
    const auto announcement = body.value(QStringLiteral("announcement")).toString();
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (announcement.size() > 5000) {
        return error(400, 1000, QStringLiteral("invalid announcement"));
    }
    if (!memberRole(groupId, userId, &role) || role != 2) {
        return error(403, 5003, QStringLiteral("permission denied"));
    }
    if (!Models::GroupModel::updateAnnouncement(groupId, announcement)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }
    return {};
}

GroupResult GroupService::info(const QString& bearerToken, qint64 groupId) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (!activeMember(groupId, userId)) {
        return error(403, 5003, QStringLiteral("permission denied"));
    }

    QJsonObject data;
    if (!Models::GroupModel::info(groupId, &data)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }
    GroupResult result;
    result.data = data;
    return result;
}

GroupResult GroupService::members(const QString& bearerToken, qint64 groupId) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (!activeMember(groupId, userId)) {
        return error(403, 5003, QStringLiteral("permission denied"));
    }

    QJsonArray data;
    if (!Models::GroupModel::members(groupId, &data)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }
    GroupResult result;
    result.data = data;
    return result;
}

GroupResult GroupService::setRole(const QString& bearerToken, qint64 groupId, qint64 userId, const QJsonObject& body) const
{
    qint64 currentUserId = 0;
    int myRole = 0;
    int targetRole = 0;
    const auto role = body.value(QStringLiteral("role")).toInt(-1);
    if (!authenticate(bearerToken, &currentUserId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (role != 0 && role != 1) {
        return error(400, 1000, QStringLiteral("invalid role"));
    }
    if (!memberRole(groupId, currentUserId, &myRole) || myRole != 2) {
        return error(403, 5003, QStringLiteral("permission denied"));
    }
    if (!memberRole(groupId, userId, &targetRole) || targetRole == 2) {
        return error(403, 5003, QStringLiteral("permission denied"));
    }
    if (!Models::GroupModel::setRole(groupId, userId, role)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }
    return {};
}

GroupResult GroupService::kick(const QString& bearerToken, qint64 groupId, qint64 userId) const
{
    qint64 currentUserId = 0;
    int myRole = 0;
    int targetRole = 0;
    if (!authenticate(bearerToken, &currentUserId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (!memberRole(groupId, currentUserId, &myRole) || !memberRole(groupId, userId, &targetRole)) {
        return error(403, 5003, QStringLiteral("permission denied"));
    }
    if (userId == currentUserId || targetRole == 2 || myRole == 0 || (myRole == 1 && targetRole != 0)) {
        return error(403, 5003, QStringLiteral("permission denied"));
    }
    if (!Models::GroupModel::removeMember(groupId, userId)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }
    return {};
}

GroupResult GroupService::mute(const QString& bearerToken, qint64 groupId, qint64 userId, const QJsonObject& body) const
{
    qint64 currentUserId = 0;
    int myRole = 0;
    int targetRole = 0;
    const auto duration = body.value(QStringLiteral("duration")).toInt(-1);
    if (!authenticate(bearerToken, &currentUserId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (duration < 0 || duration > 86400) {
        return error(400, 1000, QStringLiteral("invalid mute duration"));
    }
    if (!memberRole(groupId, currentUserId, &myRole) || !memberRole(groupId, userId, &targetRole)) {
        return error(403, 5003, QStringLiteral("permission denied"));
    }
    if (userId == currentUserId || targetRole == 2 || myRole == 0 || (myRole == 1 && targetRole != 0)) {
        return error(403, 5003, QStringLiteral("permission denied"));
    }
    if (!Models::GroupModel::muteMember(groupId, userId, duration)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }
    return {};
}

GroupResult GroupService::leave(const QString& bearerToken, qint64 groupId) const
{
    qint64 userId = 0;
    int role = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (!memberRole(groupId, userId, &role)) {
        return error(403, 5003, QStringLiteral("permission denied"));
    }
    if (role == 2) {
        return error(400, 5004, QStringLiteral("group owner cannot leave group"));
    }
    if (!Models::GroupModel::removeMember(groupId, userId)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }
    return {};
}

GroupResult GroupService::sendMessage(const QString& bearerToken, qint64 groupId, const QJsonObject& body) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (!activeMember(groupId, userId)) {
        return error(403, 5003, QStringLiteral("permission denied"));
    }

    const auto content = body.value(QStringLiteral("content")).toString();
    const auto type = body.value(QStringLiteral("type")).toInt(0);
    if (content.trimmed().isEmpty() || content.size() > 5000 || type < 0 || type > 2) {
        return error(400, 1000, QStringLiteral("invalid message payload"));
    }

    Models::GroupMuteState mute;
    if (!Models::GroupMessageModel::muteState(groupId, userId, &mute)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }
    if (mute.muted) {
        GroupResult result = error(403, 5005, QStringLiteral("您已被禁言，禁言到期时间：%1").arg(mute.muteUntil));
        result.data = QJsonObject {
            { QStringLiteral("muteUntil"), mute.muteUntil }
        };
        return result;
    }

    qint64 messageId = 0;
    QString createdAt;
    if (!Models::GroupMessageModel::insert(groupId, userId, content, type, &messageId, &createdAt)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }

    GroupResult result;
    result.data = QJsonObject {
        { QStringLiteral("messageId"), messageId },
        { QStringLiteral("createdAt"), createdAt }
    };
    return result;
}

GroupResult GroupService::history(const QString& bearerToken, qint64 groupId, int page, int pageSize) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (!activeMember(groupId, userId)) {
        return error(403, 5003, QStringLiteral("permission denied"));
    }
    if (page <= 0 || pageSize <= 0 || pageSize > 100) {
        return error(400, 1000, QStringLiteral("invalid query parameters"));
    }

    int total = 0;
    QJsonArray list;
    if (!Models::GroupMessageModel::history(groupId, userId, page, pageSize, &total, &list)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }

    GroupResult result;
    result.data = QJsonObject {
        { QStringLiteral("total"), total },
        { QStringLiteral("page"), page },
        { QStringLiteral("pageSize"), pageSize },
        { QStringLiteral("list"), list }
    };
    return result;
}

GroupResult GroupService::listMine(const QString& bearerToken) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }

    QJsonArray data;
    if (!Models::GroupModel::listMine(userId, &data)) {
        return error(503, 2002, QStringLiteral("database_error"));
    }

    GroupResult result;
    result.data = data;
    return result;
}

bool GroupService::authenticate(const QString& bearerToken, qint64* userId)
{
    return Models::MessageModel::findUserIdByToken(tokenFromBearer(bearerToken), userId);
}

GroupResult GroupService::error(int httpStatus, int code, const QString& message)
{
    GroupResult result;
    result.httpStatus = httpStatus;
    result.code = code;
    result.message = message;
    result.data = QJsonValue::Undefined;
    return result;
}

} // namespace Backend::Services
