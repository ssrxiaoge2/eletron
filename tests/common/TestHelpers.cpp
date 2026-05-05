#include "TestHelpers.h"

namespace {
constexpr auto DefaultPassword = "Test123456";
}

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

QMap<QString, TestUser> createDefaultUsers(ApiClient& client)
{
    return {
        { QStringLiteral("A"), registerAndLogin(client, QStringLiteral("test_a"), QString::fromLatin1(DefaultPassword)) },
        { QStringLiteral("B"), registerAndLogin(client, QStringLiteral("test_b"), QString::fromLatin1(DefaultPassword)) },
        { QStringLiteral("C"), registerAndLogin(client, QStringLiteral("test_c"), QString::fromLatin1(DefaultPassword)) },
        { QStringLiteral("D"), registerAndLogin(client, QStringLiteral("test_d"), QString::fromLatin1(DefaultPassword)) }
    };
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

qint64 createGroup(ApiClient& client, const TestUser& owner, const QList<TestUser>& members, const QString& name)
{
    QJsonArray ids;
    for (const auto& member : members) {
        ids.append(member.userId);
    }
    const auto response = client.post(QStringLiteral("/api/v1/groups"), {
        { QStringLiteral("name"), name },
        { QStringLiteral("memberIds"), ids }
    }, owner.token);
    return response.data().toObject().value(QStringLiteral("groupId")).toVariant().toLongLong();
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

QJsonObject findObjectByUserId(const QJsonArray& array, qint64 userId)
{
    for (const auto& item : array) {
        const auto object = item.toObject();
        if (object.value(QStringLiteral("userId")).toVariant().toLongLong() == userId
            || object.value(QStringLiteral("targetUserId")).toVariant().toLongLong() == userId
            || object.value(QStringLiteral("fromUserId")).toVariant().toLongLong() == userId) {
            return object;
        }
    }
    return {};
}

QJsonObject findGroupByName(const QJsonArray& array, const QString& name)
{
    for (const auto& item : array) {
        const auto object = item.toObject();
        if (object.value(QStringLiteral("name")).toString() == name) {
            return object;
        }
    }
    return {};
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
