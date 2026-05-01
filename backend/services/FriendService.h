#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace Backend::Services {

struct FriendResult {
    int httpStatus = 200;
    int code = 0;
    QString message = QStringLiteral("ok");
    QJsonValue data = QJsonObject {};
};

class FriendService final {
public:
    FriendResult searchUsers(const QString& bearerToken, const QString& keyword) const;
    FriendResult apply(const QString& bearerToken, qint64 targetUserId) const;
    FriendResult requests(const QString& bearerToken) const;
    FriendResult handle(const QString& bearerToken, qint64 requestId, int action) const;
    FriendResult friends(const QString& bearerToken) const;
    FriendResult startConversation(const QString& bearerToken, qint64 targetUserId) const;

private:
    static FriendResult error(int httpStatus, int code, const QString& message);
    static bool authenticate(const QString& bearerToken, qint64* userId);
};

} // namespace Backend::Services
