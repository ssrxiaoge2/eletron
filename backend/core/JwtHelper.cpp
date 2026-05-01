#include "JwtHelper.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

namespace Backend::Core {

QString JwtHelper::createToken(qint64 userId,
                               const QString& username,
                               const QString& secret,
                               int validDays)
{
    if (secret.isEmpty()) {
        return {};
    }

    const auto now = QDateTime::currentSecsSinceEpoch();
    const QJsonObject header {
        { QStringLiteral("alg"), QStringLiteral("HS256") },
        { QStringLiteral("typ"), QStringLiteral("JWT") }
    };
    const QJsonObject payload {
        { QStringLiteral("sub"), QString::number(userId) },
        { QStringLiteral("uid"), userId },
        { QStringLiteral("username"), username },
        { QStringLiteral("iat"), now },
        { QStringLiteral("exp"), now + validDays * 24 * 60 * 60 }
    };

    const auto encodedHeader = base64Url(QJsonDocument(header).toJson(QJsonDocument::Compact));
    const auto encodedPayload = base64Url(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    const auto signingInput = encodedHeader + "." + encodedPayload;
    const auto signature = base64Url(hmacSha256(secret.toUtf8(), signingInput));

    return QString::fromUtf8(signingInput + "." + signature);
}

QString JwtHelper::sha256Hex(const QString& value)
{
    return sha256Hex(value.toUtf8());
}

QString JwtHelper::sha256Hex(const QByteArray& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(value, QCryptographicHash::Sha256).toHex());
}

QByteArray JwtHelper::hmacSha256(const QByteArray& key, const QByteArray& message)
{
    constexpr int blockSize = 64;
    QByteArray normalizedKey = key;
    if (normalizedKey.size() > blockSize) {
        normalizedKey = QCryptographicHash::hash(normalizedKey, QCryptographicHash::Sha256);
    }
    normalizedKey = normalizedKey.leftJustified(blockSize, '\0', true);

    QByteArray outerPad(blockSize, char(0x5c));
    QByteArray innerPad(blockSize, char(0x36));
    for (int i = 0; i < blockSize; ++i) {
        outerPad[i] = char(outerPad[i] ^ normalizedKey[i]);
        innerPad[i] = char(innerPad[i] ^ normalizedKey[i]);
    }

    const auto innerHash = QCryptographicHash::hash(innerPad + message, QCryptographicHash::Sha256);
    return QCryptographicHash::hash(outerPad + innerHash, QCryptographicHash::Sha256);
}

QByteArray JwtHelper::base64Url(const QByteArray& value)
{
    return value.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

} // namespace Backend::Core
