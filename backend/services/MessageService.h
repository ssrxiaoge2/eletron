#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace Backend::Services {

struct MessageResult {
    int httpStatus = 200;
    int code = 0;
    QString message = QStringLiteral("ok");
    QJsonValue data;
};

class MessageService final {
public:
    MessageResult conversations(const QString& bearerToken) const;
    MessageResult history(const QString& bearerToken, qint64 targetUserId, int page, int pageSize) const;
    MessageResult sendMessage(const QString& bearerToken, qint64 receiverId, const QString& content, int type) const;
    MessageResult markRead(const QString& bearerToken, qint64 targetUserId) const;

private:
    static MessageResult error(int httpStatus, int code, const QString& message);
    static bool authenticate(const QString& bearerToken, qint64* userId);
};

} // namespace Backend::Services
