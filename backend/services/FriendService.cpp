#include "FriendService.h"

#include "../models/FriendshipModel.h"
#include "../models/MessageModel.h"

#include <QJsonArray>
#include <QJsonObject>

namespace Backend::Services {

FriendResult FriendService::searchUsers(const QString& bearerToken, const QString& keyword) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (keyword.trimmed().isEmpty()) {
        FriendResult result;
        result.data = QJsonArray {};
        return result;
    }

    QJsonArray users;
    if (!Models::FriendshipModel::searchUsers(userId, keyword.trimmed(), &users)) {
        return error(503, 2001, QStringLiteral("database_error"));
    }

    FriendResult result;
    result.data = users;
    return result;
}

FriendResult FriendService::apply(const QString& bearerToken, qint64 targetUserId) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (targetUserId <= 0) {
        return error(400, 1000, QStringLiteral("invalid targetUserId"));
    }

    int businessCode = 0;
    if (!Models::FriendshipModel::apply(userId, targetUserId, &businessCode)) {
        return error(503, 2001, QStringLiteral("database_error"));
    }
    if (businessCode == 2002) {
        return error(409, 2002, QStringLiteral("already friends"));
    }
    if (businessCode == 2003) {
        return error(409, 2003, QStringLiteral("friend request already sent"));
    }
    if (businessCode == 2004) {
        return error(400, 2004, QStringLiteral("cannot add yourself"));
    }

    FriendResult result;
    return result;
}

FriendResult FriendService::requests(const QString& bearerToken) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }

    QJsonArray requests;
    if (!Models::FriendshipModel::fetchRequests(userId, &requests)) {
        return error(503, 2001, QStringLiteral("database_error"));
    }

    FriendResult result;
    result.data = requests;
    return result;
}

FriendResult FriendService::handle(const QString& bearerToken, qint64 requestId, int action) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (requestId <= 0 || (action != 1 && action != 2)) {
        return error(400, 1000, QStringLiteral("invalid request payload"));
    }

    if (!Models::FriendshipModel::handleRequest(userId, requestId, action)) {
        return error(503, 2001, QStringLiteral("database_error"));
    }

    FriendResult result;
    return result;
}

FriendResult FriendService::friends(const QString& bearerToken) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }

    QJsonArray friends;
    if (!Models::FriendshipModel::fetchFriends(userId, &friends)) {
        return error(503, 2001, QStringLiteral("database_error"));
    }

    FriendResult result;
    result.data = friends;
    return result;
}

FriendResult FriendService::startConversation(const QString& bearerToken, qint64 targetUserId) const
{
    qint64 userId = 0;
    if (!authenticate(bearerToken, &userId)) {
        return error(401, 1002, QStringLiteral("invalid or expired token"));
    }
    if (targetUserId <= 0) {
        return error(400, 1000, QStringLiteral("invalid targetUserId"));
    }

    QJsonObject conversation;
    if (!Models::FriendshipModel::ensureConversation(userId, targetUserId, &conversation)) {
        return error(503, 2001, QStringLiteral("database_error"));
    }

    FriendResult result;
    result.data = conversation;
    return result;
}

FriendResult FriendService::error(int httpStatus, int code, const QString& message)
{
    FriendResult result;
    result.httpStatus = httpStatus;
    result.code = code;
    result.message = message;
    result.data = QJsonValue::Undefined;
    return result;
}

bool FriendService::authenticate(const QString& bearerToken, qint64* userId)
{
    const auto trimmed = bearerToken.trimmed();
    if (!trimmed.startsWith(QStringLiteral("Bearer "), Qt::CaseInsensitive)) {
        return false;
    }

    const auto token = trimmed.mid(QStringLiteral("Bearer ").size()).trimmed();
    return Models::MessageModel::findUserIdByToken(token, userId);
}

} // namespace Backend::Services
