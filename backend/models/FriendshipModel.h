#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace Backend::Models {

class FriendshipModel final {
public:
    static bool searchUsers(qint64 currentUserId, const QString& keyword, QJsonArray* users);
    static bool apply(qint64 currentUserId, qint64 targetUserId, int* businessCode);
    static bool fetchRequests(qint64 currentUserId, QJsonArray* requests);
    static bool handleRequest(qint64 currentUserId, qint64 requestId, int action);
    static bool fetchFriends(qint64 currentUserId, QJsonArray* friends);
    static bool fetchConversationTarget(qint64 currentUserId, qint64 targetUserId, QJsonObject* conversation);
};

} // namespace Backend::Models
