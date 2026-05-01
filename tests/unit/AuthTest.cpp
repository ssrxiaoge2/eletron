#include "../common/ApiClient.h"
#include "../common/TestDatabase.h"
#include "../common/TestHelpers.h"

#include <QJsonObject>
#include <QObject>
#include <QTest>

class AuthTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testRegisterSuccess();
    void testRegisterDuplicate();
    void testLoginSuccess();
    void testLoginWrongPassword();
    void testTokenValid();
    void testTokenInvalid();
};

void AuthTest::init()
{
    QVERIFY2(TestDatabase::initDb(), "TestDatabase::initDb() failed");
}

void AuthTest::cleanup()
{
    QVERIFY2(TestDatabase::cleanDb(), "TestDatabase::cleanDb() failed");
}

void AuthTest::testRegisterSuccess()
{
    ApiClient client;
    const auto response = client.post(QStringLiteral("/api/v1/auth/register"), {
        { QStringLiteral("username"), QStringLiteral("testuser1") },
        { QStringLiteral("password"), QStringLiteral("123456") }
    });

    QCOMPARE(response.code(), 0);
    QVERIFY(response.data().toObject().contains(QStringLiteral("userId")));
    QVERIFY(response.data().toObject().value(QStringLiteral("userId")).toVariant().toLongLong() > 0);
}

void AuthTest::testRegisterDuplicate()
{
    ApiClient client;
    const QJsonObject payload {
        { QStringLiteral("username"), QStringLiteral("testuser1") },
        { QStringLiteral("password"), QStringLiteral("123456") }
    };

    QCOMPARE(client.post(QStringLiteral("/api/v1/auth/register"), payload).code(), 0);
    QCOMPARE(client.post(QStringLiteral("/api/v1/auth/register"), payload).code(), 1004);
}

void AuthTest::testLoginSuccess()
{
    ApiClient client;
    QCOMPARE(client.post(QStringLiteral("/api/v1/auth/register"), {
        { QStringLiteral("username"), QStringLiteral("testuser1") },
        { QStringLiteral("password"), QStringLiteral("123456") }
    }).code(), 0);

    const auto response = client.post(QStringLiteral("/api/v1/auth/login"), {
        { QStringLiteral("username"), QStringLiteral("testuser1") },
        { QStringLiteral("password"), QStringLiteral("123456") }
    });
    const auto data = response.data().toObject();

    QCOMPARE(response.code(), 0);
    QVERIFY(!data.value(QStringLiteral("token")).toString().isEmpty());
    QVERIFY(data.value(QStringLiteral("userId")).toVariant().toLongLong() > 0);
}

void AuthTest::testLoginWrongPassword()
{
    ApiClient client;
    QCOMPARE(client.post(QStringLiteral("/api/v1/auth/register"), {
        { QStringLiteral("username"), QStringLiteral("testuser1") },
        { QStringLiteral("password"), QStringLiteral("123456") }
    }).code(), 0);

    const auto response = client.post(QStringLiteral("/api/v1/auth/login"), {
        { QStringLiteral("username"), QStringLiteral("testuser1") },
        { QStringLiteral("password"), QStringLiteral("wrong-password") }
    });

    QCOMPARE(response.code(), 1001);
}

void AuthTest::testTokenValid()
{
    ApiClient client;
    const auto user = registerAndLogin(client, QStringLiteral("testuser1"));
    QVERIFY(user.userId > 0);
    QVERIFY(!user.token.isEmpty());

    const auto response = client.get(QStringLiteral("/api/v1/user/profile"), user.token);
    const auto data = response.data().toObject();

    QCOMPARE(response.code(), 0);
    QCOMPARE(data.value(QStringLiteral("userId")).toVariant().toLongLong(), user.userId);
    QCOMPARE(data.value(QStringLiteral("username")).toString(), QStringLiteral("testuser1"));
}

void AuthTest::testTokenInvalid()
{
    ApiClient client;
    const auto response = client.get(QStringLiteral("/api/v1/user/profile"), QStringLiteral("fake.jwt.token"));
    QCOMPARE(response.code(), 1002);
}

int runAuthTest(int argc, char** argv)
{
    AuthTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "AuthTest.moc"
