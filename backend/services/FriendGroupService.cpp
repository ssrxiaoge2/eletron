#include "FriendGroupService.h"

#include "../models/FriendGroupModel.h"
#include "../models/MessageModel.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QVariant>

namespace Backend::Services {
namespace {

QString normalizedName(const QJsonObject& body)
{
    return body.value(QStringLiteral("name")).toString().trimmed();
}

FriendGroupResult businessError(int businessCode)
{
    if (businessCode == 6001) {
        return FriendGroupService::error(409, 6001, QStringLiteral("friend group name already exists"));
    }
    if (businessCode == 6002) {
        return FriendGroupService::error(400, 6002, QStringLiteral("default friend group cannot be deleted"));
    }
    return FriendGroupService::error(404, 6003, QStringLiteral("friend group not found or no permission"));
}

} // namespace

FriendGroupResult FriendGroupService::list(const QString& bearerToken) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }

    QJsonArray groups;
    if (!Models::FriendGroupModel::listWithFriends(userId, &groups)) {
        return error(503, 2001, QStringLiteral("database_error"));
    }

    FriendGroupResult result;
    result.data = groups;
    return result;
}

FriendGroupResult FriendGroupService::create(const QString& bearerToken, const QJsonObject& body) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    const auto name = normalizedName(body);
    if (name.isEmpty() || name.size() > 32) {
        return error(400, 1000, QStringLiteral("invalid request payload"));
    }

    int businessCode = 0;
    QJsonObject group;
    if (!Models::FriendGroupModel::create(userId, name, &group, &businessCode)) {
        return error(503, 2001, QStringLiteral("database_error"));
    }
    if (businessCode != 0) {
        return businessError(businessCode);
    }

    FriendGroupResult result;
    result.data = group;
    return result;
}

FriendGroupResult FriendGroupService::rename(const QString& bearerToken, qint64 groupId, const QJsonObject& body) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    const auto name = normalizedName(body);
    if (groupId <= 0 || name.isEmpty() || name.size() > 32) {
        return error(400, 1000, QStringLiteral("invalid request payload"));
    }

    int businessCode = 0;
    if (!Models::FriendGroupModel::rename(userId, groupId, name, &businessCode)) {
        return error(503, 2001, QStringLiteral("database_error"));
    }
    if (businessCode != 0) {
        return businessError(businessCode);
    }

    FriendGroupResult result;
    result.data = QJsonObject {
        { QStringLiteral("groupId"), groupId },
        { QStringLiteral("name"), name }
    };
    return result;
}

FriendGroupResult FriendGroupService::remove(const QString& bearerToken, qint64 groupId) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (groupId <= 0) {
        return error(400, 1000, QStringLiteral("invalid groupId"));
    }

    int businessCode = 0;
    if (!Models::FriendGroupModel::remove(userId, groupId, &businessCode)) {
        return error(503, 2001, QStringLiteral("database_error"));
    }
    if (businessCode != 0) {
        return businessError(businessCode);
    }

    return FriendGroupResult {};
}

FriendGroupResult FriendGroupService::moveFriend(const QString& bearerToken, qint64 friendUserId, const QJsonObject& body) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    const auto groupId = body.value(QStringLiteral("groupId")).toVariant().toLongLong();
    if (friendUserId <= 0 || groupId <= 0) {
        return error(400, 1000, QStringLiteral("invalid request payload"));
    }

    int businessCode = 0;
    if (!Models::FriendGroupModel::moveFriend(userId, friendUserId, groupId, &businessCode)) {
        return error(503, 2001, QStringLiteral("database_error"));
    }
    if (businessCode != 0) {
        return businessError(businessCode);
    }

    return FriendGroupResult {};
}

FriendGroupResult FriendGroupService::error(int httpStatus, int code, const QString& message)
{
    FriendGroupResult result;
    result.httpStatus = httpStatus;
    result.code = code;
    result.message = message;
    result.data = QJsonValue::Undefined;
    return result;
}

bool FriendGroupService::authenticate(const QString& bearerToken, qint64* userId)
{
    const auto trimmed = bearerToken.trimmed();
    if (!trimmed.startsWith(QStringLiteral("Bearer "), Qt::CaseInsensitive)) {
        return false;
    }
    const auto token = trimmed.mid(QStringLiteral("Bearer ").size()).trimmed();
    return Models::MessageModel::findUserIdByToken(token, userId);
}

} // namespace Backend::Services
