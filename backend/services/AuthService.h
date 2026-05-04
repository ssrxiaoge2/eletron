#pragma once

#include <QJsonObject>
#include <QString>

namespace Backend::Services {

struct AuthResult {
    int httpStatus = 200;
    int code = 0;
    QString message = QStringLiteral("ok");
    QJsonObject data;
};

class AuthService final {
public:
    AuthResult registerUser(const QString& username, const QString& password) const;
    AuthResult login(const QString& username, const QString& password) const;
    AuthResult logout(const QString& bearerToken) const;

private:
    static AuthResult error(int httpStatus, int code, const QString& message);
    static QString hashPassword(const QString& password);
    static QString extractBearerToken(const QString& bearerToken);
};

} // namespace Backend::Services
