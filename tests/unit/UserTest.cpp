#include "../common/ApiClient.h"
#include "../common/TestDatabase.h"
#include "../common/TestHelpers.h"

#include <QObject>
#include <QTest>

class UserTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testGetProfile();
    void testUpdateProfile();
    void testUpdateProfilePartial();
};

void UserTest::init()
{
    QVERIFY2(TestDatabase::initDb(), "TestDatabase::initDb() failed");
}

void UserTest::cleanup()
{
    QVERIFY2(TestDatabase::cleanDb(), "TestDatabase::cleanDb() failed");
}

void UserTest::testGetProfile()
{
    ApiClient client;
    const auto user = registerAndLogin(client, QStringLiteral("testuser1"));
    QVERIFY(user.userId > 0);

    const auto response = client.get(QStringLiteral("/api/v1/user/profile"), user.token);
    const auto data = response.data().toObject();

    QCOMPARE(response.code(), 0);
    QVERIFY(data.contains(QStringLiteral("userId")));
    QVERIFY(data.contains(QStringLiteral("username")));
    QVERIFY(data.contains(QStringLiteral("nickname")));
    QVERIFY(data.contains(QStringLiteral("signature")));
    QVERIFY(data.contains(QStringLiteral("email")));
}

void UserTest::testUpdateProfile()
{
    ApiClient client;
    const auto user = registerAndLogin(client, QStringLiteral("testuser1"));
    QVERIFY(user.userId > 0);

    const auto response = client.put(QStringLiteral("/api/v1/user/profile"), {
        { QStringLiteral("nickname"), QStringLiteral("新昵称") },
        { QStringLiteral("signature"), QStringLiteral("新签名") }
    }, user.token);
    const auto data = response.data().toObject();

    QCOMPARE(response.code(), 0);
    QCOMPARE(data.value(QStringLiteral("nickname")).toString(), QStringLiteral("新昵称"));
    QCOMPARE(data.value(QStringLiteral("signature")).toString(), QStringLiteral("新签名"));
}

void UserTest::testUpdateProfilePartial()
{
    ApiClient client;
    const auto user = registerAndLogin(client, QStringLiteral("testuser1"));
    QVERIFY(user.userId > 0);

    const auto before = client.get(QStringLiteral("/api/v1/user/profile"), user.token).data().toObject();
    const auto response = client.put(QStringLiteral("/api/v1/user/profile"), {
        { QStringLiteral("nickname"), QStringLiteral("只改昵称") }
    }, user.token);
    const auto data = response.data().toObject();

    QCOMPARE(response.code(), 0);
    QCOMPARE(data.value(QStringLiteral("nickname")).toString(), QStringLiteral("只改昵称"));
    QCOMPARE(data.value(QStringLiteral("signature")).toString(), before.value(QStringLiteral("signature")).toString());
    QCOMPARE(data.value(QStringLiteral("avatar")).toString(), before.value(QStringLiteral("avatar")).toString());
    QCOMPARE(data.value(QStringLiteral("gender")).toString(), before.value(QStringLiteral("gender")).toString());
    QCOMPARE(data.value(QStringLiteral("birthday")).toString(), before.value(QStringLiteral("birthday")).toString());
}

int runUserTest(int argc, char** argv)
{
    UserTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "UserTest.moc"
