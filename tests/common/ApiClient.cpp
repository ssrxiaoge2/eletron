#include "ApiClient.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
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

ApiResponse ApiClient::send(const QString& method,
                            const QString& path,
                            const QJsonObject* payload,
                            const QString& token)
{
    QNetworkRequest request(QUrl(baseUrl_ + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    }

    const auto body = payload ? QJsonDocument(*payload).toJson(QJsonDocument::Compact) : QByteArray();
    QNetworkReply* reply = nullptr;
    if (method == QStringLiteral("GET")) {
        reply = manager_.get(request);
    } else if (method == QStringLiteral("POST")) {
        reply = manager_.post(request, body);
    } else if (method == QStringLiteral("PUT")) {
        reply = manager_.put(request, body);
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
    timer.start(10000);
    loop.exec();

    ApiResponse response;
    if (!timer.isActive()) {
        response.error = QStringLiteral("request timed out");
        reply->abort();
        reply->deleteLater();
        return response;
    }

    response.httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
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
