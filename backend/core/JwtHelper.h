#pragma once

#include <QString>

namespace Backend::Core {

class JwtHelper final {
public:
    static QString createToken(qint64 userId,
                               const QString& username,
                               const QString& secret,
                               int validDays = 7);
    static QString sha256Hex(const QString& value);
    static QString sha256Hex(const QByteArray& value);

private:
    static QByteArray hmacSha256(const QByteArray& key, const QByteArray& message);
    static QByteArray base64Url(const QByteArray& value);
};

} // namespace Backend::Core
