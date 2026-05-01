#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace Backend::Services {

struct UserResult {
    int httpStatus = 200;
    int code = 0;
    QString message = QStringLiteral("ok");
    QJsonValue data = QJsonObject {};
};

class UserService final {
public:
    UserResult profile(const QString& bearerToken) const;
    UserResult updateProfile(const QString& bearerToken, const QJsonObject& body) const;
    UserResult uploadAvatar(const QString& bearerToken, const QJsonObject& body) const;

private:
    static UserResult error(int httpStatus, int code, const QString& message);
    static bool authenticate(const QString& bearerToken, qint64* userId);
    static bool validateBody(const QJsonObject& body);
};

} // namespace Backend::Services
