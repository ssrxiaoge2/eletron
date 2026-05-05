#include "ApiClient.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUuid>
#include <QTimer>
#include <QUrl>

int ApiResponse::code() const
{
    return body.value(QStringLiteral("code")).toInt(-1);
}

QJsonValue ApiResponse::data() const
{
    return body.value(QStringLiteral("data"));
}

ApiClient::ApiClient(QString baseUrl)
    : baseUrl_(std::move(baseUrl))
{
}

ApiResponse ApiClient::get(const QString& path, const QString& token)
{
    return send(QStringLiteral("GET"), path, nullptr, token);
}

ApiResponse ApiClient::post(const QString& path, const QJsonObject& payload, const QString& token)
{
    return send(QStringLiteral("POST"), path, &payload, token);
}

ApiResponse ApiClient::put(const QString& path, const QJsonObject& payload, const QString& token)
{
    return send(QStringLiteral("PUT"), path, &payload, token);
}

ApiResponse ApiClient::deleteResource(const QString& path, const QString& token)
{
    return send(QStringLiteral("DELETE"), path, nullptr, token);
}

ApiResponse ApiClient::postMultipart(const QString& path,
                                     const QString& token,
                                     const QString& fileFieldName,
                                     const QString& fileName,
                                     const QByteArray& fileBytes,
                                     const QString& mimeType,
                                     const QVariantMap& fields)
{
    const auto boundary = QStringLiteral("----im-test-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QByteArray body;
    const auto boundaryLine = QByteArray("--") + boundary.toUtf8() + "\r\n";

    for (auto it = fields.cbegin(); it != fields.cend(); ++it) {
        body += boundaryLine;
        body += "Content-Disposition: form-data; name=\"" + it.key().toUtf8() + "\"\r\n\r\n";
        body += it.value().toString().toUtf8();
        body += "\r\n";
    }

    body += boundaryLine;
    body += "Content-Disposition: form-data; name=\"" + fileFieldName.toUtf8()
        + "\"; filename=\"" + fileName.toUtf8() + "\"\r\n";
    body += "Content-Type: " + mimeType.toUtf8() + "\r\n\r\n";
    body += fileBytes;
    body += "\r\n--" + boundary.toUtf8() + "--\r\n";

    return sendRaw(QStringLiteral("POST"),
                   path,
                   body,
                   QStringLiteral("multipart/form-data; boundary=%1").arg(boundary),
                   token);
}

ApiResponse ApiClient::send(const QString& method,
                            const QString& path,
                            const QJsonObject* payload,
                            const QString& token)
{
    return sendRaw(method,
                   path,
                   payload ? QJsonDocument(*payload).toJson(QJsonDocument::Compact) : QByteArray(),
                   QStringLiteral("application/json"),
                   token);
}

ApiResponse ApiClient::sendRaw(const QString& method,
                               const QString& path,
                               const QByteArray& body,
                               const QString& contentType,
                               const QString& token)
{
    QNetworkRequest request(QUrl(baseUrl_ + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    }

    QNetworkReply* reply = nullptr;
    if (method == QStringLiteral("GET")) {
        reply = manager_.get(request);
    } else if (method == QStringLiteral("POST")) {
        reply = manager_.post(request, body);
    } else if (method == QStringLiteral("PUT")) {
        reply = manager_.put(request, body);
    } else if (method == QStringLiteral("DELETE")) {
        reply = manager_.deleteResource(request);
    } else {
        ApiResponse response;
        response.error = QStringLiteral("unsupported method");
        return response;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(60000);
    loop.exec();

    ApiResponse response;
    if (!timer.isActive()) {
        response.error = QStringLiteral("request timed out");
        reply->abort();
        reply->deleteLater();
        return response;
    }

    response.httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    response.contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    response.rawBody = reply->readAll();
    if (reply->error() != QNetworkReply::NoError
        && response.httpStatus == 0) {
        response.error = reply->errorString();
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(response.rawBody, &parseError);
    if (parseError.error == QJsonParseError::NoError && document.isObject()) {
        response.body = document.object();
    } else if (!response.rawBody.isEmpty()) {
        response.error = QStringLiteral("invalid json response: %1").arg(parseError.errorString());
    }

    reply->deleteLater();
    return response;
}
