#include "TestHelpers.h"

TestUser registerAndLogin(ApiClient& client, const QString& username, const QString& password)
{
    TestUser user;
    user.username = username;
    user.password = password;

    const auto registerResponse = client.post(QStringLiteral("/api/v1/auth/register"), {
        { QStringLiteral("username"), username },
        { QStringLiteral("password"), password }
    });
    if (registerResponse.code() != 0) {
        return user;
    }
    user.userId = registerResponse.data().toObject().value(QStringLiteral("userId")).toVariant().toLongLong();

    const auto loginResponse = client.post(QStringLiteral("/api/v1/auth/login"), {
        { QStringLiteral("username"), username },
        { QStringLiteral("password"), password }
    });
    if (loginResponse.code() != 0) {
        user.userId = 0;
        return user;
    }
    user.token = loginResponse.data().toObject().value(QStringLiteral("token")).toString();
    return user;
}

bool makeFriends(ApiClient& client, const TestUser& from, const TestUser& to)
{
    const auto applyResponse = client.post(QStringLiteral("/api/v1/friends/apply"), {
        { QStringLiteral("targetUserId"), to.userId }
    }, from.token);
    if (applyResponse.code() != 0) {
        return false;
    }

    const auto requestsResponse = client.get(QStringLiteral("/api/v1/friends/requests"), to.token);
    const auto requestId = firstRequestIdFrom(requestsResponse.data().toArray(), from.userId);
    if (requestId <= 0) {
        return false;
    }

    const auto handleResponse = client.post(QStringLiteral("/api/v1/friends/handle"), {
        { QStringLiteral("requestId"), requestId },
        { QStringLiteral("action"), 1 }
    }, to.token);
    return handleResponse.code() == 0;
}

bool arrayContainsUserId(const QJsonArray& array, qint64 userId)
{
    for (const auto& item : array) {
        const auto object = item.toObject();
        if (object.value(QStringLiteral("userId")).toVariant().toLongLong() == userId
            || object.value(QStringLiteral("targetUserId")).toVariant().toLongLong() == userId) {
            return true;
        }
    }
    return false;
}

qint64 firstRequestIdFrom(const QJsonArray& array, qint64 fromUserId)
{
    for (const auto& item : array) {
        const auto object = item.toObject();
        if (object.value(QStringLiteral("fromUserId")).toVariant().toLongLong() == fromUserId) {
            return object.value(QStringLiteral("requestId")).toVariant().toLongLong();
        }
    }
    return 0;
}
