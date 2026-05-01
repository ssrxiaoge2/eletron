#include "../common/ApiClient.h"
#include "../common/TestDatabase.h"
#include "../common/TestHelpers.h"

#include <QJsonArray>
#include <QObject>
#include <QTest>

class FriendTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testSearchUser();
    void testSearchNoResult();
    void testApplyFriend();
    void testApplyFriendDuplicate();
    void testApplySelf();
    void testGetFriendRequests();
    void testAcceptFriendRequest();
    void testGetFriendList();
    void testRejectFriendRequest();
};

void FriendTest::init()
{
    QVERIFY2(TestDatabase::initDb(), "TestDatabase::initDb() failed");
}

void FriendTest::cleanup()
{
    QVERIFY2(TestDatabase::cleanDb(), "TestDatabase::cleanDb() failed");
}

void FriendTest::testSearchUser()
{
    ApiClient client;
    const auto userA = registerAndLogin(client, QStringLiteral("testuserA"));
    const auto userB = registerAndLogin(client, QStringLiteral("testuserB"));
    QVERIFY(userA.userId > 0);
    QVERIFY(userB.userId > 0);

    const auto response = client.get(QStringLiteral("/api/v1/users/search?keyword=test"), userA.token);
    const auto users = response.data().toArray();

    QCOMPARE(response.code(), 0);
    QVERIFY(arrayContainsUserId(users, userB.userId));
    QVERIFY(!arrayContainsUserId(users, userA.userId));
}

void FriendTest::testSearchNoResult()
{
    ApiClient client;
    const auto user = registerAndLogin(client, QStringLiteral("testuser1"));
    QVERIFY(user.userId > 0);

    const auto response = client.get(QStringLiteral("/api/v1/users/search?keyword=no_such_user_keyword"), user.token);
    QCOMPARE(response.code(), 0);
    QVERIFY(response.data().toArray().isEmpty());
}

void FriendTest::testApplyFriend()
{
    ApiClient client;
    const auto userA = registerAndLogin(client, QStringLiteral("testuserA"));
    const auto userB = registerAndLogin(client, QStringLiteral("testuserB"));
    QVERIFY(userA.userId > 0);
    QVERIFY(userB.userId > 0);

    const auto response = client.post(QStringLiteral("/api/v1/friends/apply"), {
        { QStringLiteral("targetUserId"), userB.userId }
    }, userA.token);
    QCOMPARE(response.code(), 0);
}

void FriendTest::testApplyFriendDuplicate()
{
    ApiClient client;
    const auto userA = registerAndLogin(client, QStringLiteral("testuserA"));
    const auto userB = registerAndLogin(client, QStringLiteral("testuserB"));
    QVERIFY(userA.userId > 0);
    QVERIFY(userB.userId > 0);

    const QJsonObject payload {
        { QStringLiteral("targetUserId"), userB.userId }
    };
    QCOMPARE(client.post(QStringLiteral("/api/v1/friends/apply"), payload, userA.token).code(), 0);
    QCOMPARE(client.post(QStringLiteral("/api/v1/friends/apply"), payload, userA.token).code(), 2003);
}

void FriendTest::testApplySelf()
{
    ApiClient client;
    const auto user = registerAndLogin(client, QStringLiteral("testuser1"));
    QVERIFY(user.userId > 0);

    const auto response = client.post(QStringLiteral("/api/v1/friends/apply"), {
        { QStringLiteral("targetUserId"), user.userId }
    }, user.token);
    QCOMPARE(response.code(), 2004);
}

void FriendTest::testGetFriendRequests()
{
    ApiClient client;
    const auto userA = registerAndLogin(client, QStringLiteral("testuserA"));
    const auto userB = registerAndLogin(client, QStringLiteral("testuserB"));
    QVERIFY(userA.userId > 0);
    QVERIFY(userB.userId > 0);
    QCOMPARE(client.post(QStringLiteral("/api/v1/friends/apply"), {
        { QStringLiteral("targetUserId"), userB.userId }
    }, userA.token).code(), 0);

    const auto response = client.get(QStringLiteral("/api/v1/friends/requests"), userB.token);
    QCOMPARE(response.code(), 0);
    QVERIFY(firstRequestIdFrom(response.data().toArray(), userA.userId) > 0);
}

void FriendTest::testAcceptFriendRequest()
{
    ApiClient client;
    const auto userA = registerAndLogin(client, QStringLiteral("testuserA"));
    const auto userB = registerAndLogin(client, QStringLiteral("testuserB"));
    QVERIFY(userA.userId > 0);
    QVERIFY(userB.userId > 0);
    QCOMPARE(client.post(QStringLiteral("/api/v1/friends/apply"), {
        { QStringLiteral("targetUserId"), userB.userId }
    }, userA.token).code(), 0);
    const auto requestId = firstRequestIdFrom(
        client.get(QStringLiteral("/api/v1/friends/requests"), userB.token).data().toArray(),
        userA.userId);
    QVERIFY(requestId > 0);

    const auto response = client.post(QStringLiteral("/api/v1/friends/handle"), {
        { QStringLiteral("requestId"), requestId },
        { QStringLiteral("action"), 1 }
    }, userB.token);

    QCOMPARE(response.code(), 0);
    QVERIFY(arrayContainsUserId(client.get(QStringLiteral("/api/v1/friends"), userA.token).data().toArray(), userB.userId));
    QVERIFY(arrayContainsUserId(client.get(QStringLiteral("/api/v1/friends"), userB.token).data().toArray(), userA.userId));
}

void FriendTest::testGetFriendList()
{
    ApiClient client;
    const auto userA = registerAndLogin(client, QStringLiteral("testuserA"));
    const auto userB = registerAndLogin(client, QStringLiteral("testuserB"));
    QVERIFY(userA.userId > 0);
    QVERIFY(userB.userId > 0);
    QVERIFY(makeFriends(client, userA, userB));

    const auto friendsA = client.get(QStringLiteral("/api/v1/friends"), userA.token);
    const auto friendsB = client.get(QStringLiteral("/api/v1/friends"), userB.token);
    QCOMPARE(friendsA.code(), 0);
    QCOMPARE(friendsB.code(), 0);
    QVERIFY(arrayContainsUserId(friendsA.data().toArray(), userB.userId));
    QVERIFY(arrayContainsUserId(friendsB.data().toArray(), userA.userId));
}

void FriendTest::testRejectFriendRequest()
{
    ApiClient client;
    const auto userA = registerAndLogin(client, QStringLiteral("testuserA"));
    const auto userB = registerAndLogin(client, QStringLiteral("testuserB"));
    QVERIFY(userA.userId > 0);
    QVERIFY(userB.userId > 0);
    QCOMPARE(client.post(QStringLiteral("/api/v1/friends/apply"), {
        { QStringLiteral("targetUserId"), userB.userId }
    }, userA.token).code(), 0);
    const auto requestId = firstRequestIdFrom(
        client.get(QStringLiteral("/api/v1/friends/requests"), userB.token).data().toArray(),
        userA.userId);
    QVERIFY(requestId > 0);

    const auto response = client.post(QStringLiteral("/api/v1/friends/handle"), {
        { QStringLiteral("requestId"), requestId },
        { QStringLiteral("action"), 2 }
    }, userB.token);

    QCOMPARE(response.code(), 0);
    QVERIFY(!arrayContainsUserId(client.get(QStringLiteral("/api/v1/friends"), userA.token).data().toArray(), userB.userId));
    QVERIFY(!arrayContainsUserId(client.get(QStringLiteral("/api/v1/friends"), userB.token).data().toArray(), userA.userId));
}

int runFriendTest(int argc, char** argv)
{
    FriendTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "FriendTest.moc"
