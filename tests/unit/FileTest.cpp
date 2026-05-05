#include "../common/ApiClient.h"
#include "../common/TestDatabase.h"
#include "../common/TestHelpers.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QTest>

class FileTest : public QObject {
    Q_OBJECT
private:
    ApiClient client;
    QMap<QString, TestUser> users;

    QByteArray pngBytes() const;
    ApiResponse uploadImage(const TestUser& sender, const TestUser& receiver);
    ApiResponse uploadTextFile(const TestUser& sender, const TestUser& receiver, const QByteArray& bytes = QByteArray("hello file"));
    bool historyHasFileMessage(qint64 targetUserId, const QString& token, int type, const QString& field);
private slots:
    void init();
    void cleanup();
    void testUploadImageSuccess();
    void testUploadImageTooLarge();
    void testGetThumbnail();
    void testUploadFileSuccess();
    void testUploadFileTooLarge();
    void testDownloadFileSuccess();
    void testDownloadFileByReceiver();
    void testDownloadFileByStranger();
    void testGetFileInfo();
    void testImageMessageInHistory();
    void testFileMessagePreviewInConversation();
};

void FileTest::init()
{
    QVERIFY2(TestDatabase::initDb(), "initDb failed");
    users = createDefaultUsers(client);
    QVERIFY(users["A"].userId > 0);
    QVERIFY(users["B"].userId > 0);
    QVERIFY(makeFriends(client, users["A"], users["B"]));
}

void FileTest::cleanup() { QVERIFY2(TestDatabase::cleanDb(), "cleanDb failed"); }

QByteArray FileTest::pngBytes() const
{
    return QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==");
}

ApiResponse FileTest::uploadImage(const TestUser& sender, const TestUser& receiver)
{
    return client.postMultipart(QStringLiteral("/api/v1/files/upload"), sender.token, QStringLiteral("file"),
                                QStringLiteral("tiny.png"), pngBytes(), QStringLiteral("image/png"),
                                {{QStringLiteral("type"), 1}, {QStringLiteral("receiverId"), receiver.userId}});
}

ApiResponse FileTest::uploadTextFile(const TestUser& sender, const TestUser& receiver, const QByteArray& bytes)
{
    return client.postMultipart(QStringLiteral("/api/v1/files/upload"), sender.token, QStringLiteral("file"),
                                QStringLiteral("note.txt"), bytes, QStringLiteral("text/plain"),
                                {{QStringLiteral("type"), 2}, {QStringLiteral("receiverId"), receiver.userId}});
}

bool FileTest::historyHasFileMessage(qint64 targetUserId, const QString& token, int type, const QString& field)
{
    const auto messages = client.get(QStringLiteral("/api/v1/messages?targetUserId=%1&page=1&pageSize=20").arg(targetUserId), token)
                              .data().toObject().value(QStringLiteral("list")).toArray();
    for (const auto& item : messages) {
        const auto message = item.toObject();
        if (message.value(QStringLiteral("type")).toInt() != type) {
            continue;
        }
        const auto fileDoc = QJsonDocument::fromJson(message.value(QStringLiteral("content")).toString().toUtf8());
        if (fileDoc.isObject() && fileDoc.object().contains(QStringLiteral("fileId")) && fileDoc.object().contains(field)) {
            return true;
        }
    }
    return false;
}

void FileTest::testUploadImageSuccess()
{
    const auto response = uploadImage(users["A"], users["B"]);
    const auto data = response.data().toObject();
    QCOMPARE(response.code(), 0);
    QVERIFY(data.value(QStringLiteral("fileId")).toVariant().toLongLong() > 0);
    QVERIFY(!data.value(QStringLiteral("url")).toString().isEmpty());
    QVERIFY(!data.value(QStringLiteral("thumbnailUrl")).toString().isEmpty());
}

void FileTest::testUploadImageTooLarge()
{
    const QByteArray large(20 * 1024 * 1024 + 1, 'x');
    const auto response = client.postMultipart(QStringLiteral("/api/v1/files/upload"), users["A"].token, QStringLiteral("file"),
                                               QStringLiteral("large.png"), large, QStringLiteral("image/png"),
                                               {{QStringLiteral("type"), 1}, {QStringLiteral("receiverId"), users["B"].userId}});
    QCOMPARE(response.code(), 4001);
}

void FileTest::testGetThumbnail()
{
    const qint64 fileId = uploadImage(users["A"], users["B"]).data().toObject().value(QStringLiteral("fileId")).toVariant().toLongLong();
    QVERIFY(fileId > 0);
    const auto response = client.get(QStringLiteral("/api/v1/files/thumbnail/%1").arg(fileId), users["A"].token);
    QCOMPARE(response.httpStatus, 200);
    QVERIFY(response.contentType.startsWith(QStringLiteral("image/")));
    QVERIFY(!response.rawBody.isEmpty());
}

void FileTest::testUploadFileSuccess()
{
    const auto response = uploadTextFile(users["A"], users["B"]);
    const auto data = response.data().toObject();
    QCOMPARE(response.code(), 0);
    QVERIFY(data.value(QStringLiteral("fileId")).toVariant().toLongLong() > 0);
    QVERIFY(!data.value(QStringLiteral("url")).toString().isEmpty());
    QCOMPARE(data.value(QStringLiteral("fileName")).toString(), QStringLiteral("note.txt"));
    QCOMPARE(data.value(QStringLiteral("fileSize")).toInt(), QByteArray("hello file").size());
}

void FileTest::testUploadFileTooLarge()
{
    const QByteArray large(100 * 1024 * 1024 + 1, 'x');
    const auto response = client.postMultipart(QStringLiteral("/api/v1/files/upload"), users["A"].token, QStringLiteral("file"),
                                               QStringLiteral("large.txt"), large, QStringLiteral("text/plain"),
                                               {{QStringLiteral("type"), 2}, {QStringLiteral("receiverId"), users["B"].userId}});
    QCOMPARE(response.code(), 4001);
}

void FileTest::testDownloadFileSuccess()
{
    const QByteArray source("download me");
    const qint64 fileId = uploadTextFile(users["A"], users["B"], source).data().toObject().value(QStringLiteral("fileId")).toVariant().toLongLong();
    const auto response = client.get(QStringLiteral("/api/v1/files/download/%1").arg(fileId), users["A"].token);
    QCOMPARE(response.httpStatus, 200);
    QCOMPARE(response.rawBody.size(), source.size());
}

void FileTest::testDownloadFileByReceiver()
{
    const qint64 fileId = uploadTextFile(users["A"], users["B"]).data().toObject().value(QStringLiteral("fileId")).toVariant().toLongLong();
    const auto response = client.get(QStringLiteral("/api/v1/files/download/%1").arg(fileId), users["B"].token);
    QCOMPARE(response.httpStatus, 200);
    QVERIFY(!response.rawBody.isEmpty());
}

void FileTest::testDownloadFileByStranger()
{
    const qint64 fileId = uploadTextFile(users["A"], users["B"]).data().toObject().value(QStringLiteral("fileId")).toVariant().toLongLong();
    const auto response = client.get(QStringLiteral("/api/v1/files/download/%1").arg(fileId), users["C"].token);
    QCOMPARE(response.code(), 1003);
}

void FileTest::testGetFileInfo()
{
    const qint64 fileId = uploadTextFile(users["A"], users["B"]).data().toObject().value(QStringLiteral("fileId")).toVariant().toLongLong();
    const auto data = client.get(QStringLiteral("/api/v1/files/info/%1").arg(fileId), users["A"].token).data().toObject();
    QVERIFY(data.contains(QStringLiteral("fileName")));
    QVERIFY(data.contains(QStringLiteral("fileSize")));
    QVERIFY(data.contains(QStringLiteral("fileType")));
    QVERIFY(data.contains(QStringLiteral("mimeType")));
}

void FileTest::testImageMessageInHistory()
{
    QCOMPARE(uploadImage(users["A"], users["B"]).code(), 0);
    QVERIFY(historyHasFileMessage(users["B"].userId, users["A"].token, 1, QStringLiteral("thumbnailUrl")));
}

void FileTest::testFileMessagePreviewInConversation()
{
    QCOMPARE(uploadTextFile(users["A"], users["B"]).code(), 0);
    const auto conversations = client.get(QStringLiteral("/api/v1/conversations"), users["A"].token).data().toArray();
    QVERIFY(!conversations.isEmpty());
    QCOMPARE(conversations.first().toObject().value(QStringLiteral("lastMessageType")).toInt(), 2);
    const auto fileDoc = QJsonDocument::fromJson(conversations.first().toObject().value(QStringLiteral("lastMessage")).toString().toUtf8());
    QVERIFY(fileDoc.isObject());
    QVERIFY(fileDoc.object().contains(QStringLiteral("fileId")));
}

int runFileTest(int argc, char** argv)
{
    FileTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "FileTest.moc"
