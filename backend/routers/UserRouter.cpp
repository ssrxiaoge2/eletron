#include "UserRouter.h"

#include "../services/UserService.h"

#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QMimeDatabase>

namespace Backend::Routers {
namespace {

QString authorizationHeader(const QHttpServerRequest& request)
{
    auto value = request.value(QByteArrayLiteral("Authorization"));
    if (value.isEmpty()) {
        value = request.value(QByteArrayLiteral("authorization"));
    }
    return QString::fromUtf8(value);
}

QHttpServerResponse jsonResponse(const QJsonObject& payload, int statusCode)
{
    return QHttpServerResponse(
        QByteArrayLiteral("application/json"),
        QJsonDocument(payload).toJson(QJsonDocument::Compact),
        static_cast<QHttpServerResponse::StatusCode>(statusCode));
}

QHttpServerResponse serviceResponse(const Services::UserResult& result)
{
    QJsonObject payload {
        { QStringLiteral("code"), result.code },
        { QStringLiteral("message"), result.message }
    };
    if (!result.data.isUndefined() && !result.data.isNull()) {
        payload.insert(QStringLiteral("data"), result.data);
    }
    return jsonResponse(payload, result.httpStatus);
}

QHttpServerResponse fileResponse(const QString& fileName)
{
    if (fileName.contains(QStringLiteral(".."))
        || fileName.contains(QLatin1Char('/'))
        || fileName.contains(QLatin1Char('\\'))) {
        return jsonResponse({
                                { QStringLiteral("code"), 1000 },
                                { QStringLiteral("message"), QStringLiteral("invalid file name") }
                            },
                            400);
    }

    const auto filePath = QDir::current().filePath(QStringLiteral("backend/uploads/avatars/%1").arg(fileName));
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return jsonResponse({
                                { QStringLiteral("code"), 1004 },
                                { QStringLiteral("message"), QStringLiteral("avatar not found") }
                            },
                            404);
    }

    const auto mimeType = QMimeDatabase().mimeTypeForFile(filePath).name().toUtf8();
    return QHttpServerResponse(mimeType, file.readAll(), QHttpServerResponse::StatusCode::Ok);
}

} // namespace

void UserRouter::registerRoutes(QHttpServer& server)
{
    server.route(QStringLiteral("/api/v1/user/profile"),
                 QHttpServerRequest::Method::Get,
                 [](const QHttpServerRequest& request) {
                     const Services::UserService service;
                     return serviceResponse(service.profile(authorizationHeader(request)));
                 });

    server.route(QStringLiteral("/api/v1/user/profile"),
                 QHttpServerRequest::Method::Put,
                 [](const QHttpServerRequest& request) {
                     QJsonParseError parseError;
                     const auto document = QJsonDocument::fromJson(request.body(), &parseError);
                     if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                         return jsonResponse({
                                                 { QStringLiteral("code"), 1000 },
                                                 { QStringLiteral("message"), QStringLiteral("invalid request body") }
                                             },
                                             400);
                     }

                     const Services::UserService service;
                     return serviceResponse(service.updateProfile(authorizationHeader(request), document.object()));
                 });

    server.route(QStringLiteral("/api/v1/user/avatar"),
                 QHttpServerRequest::Method::Post,
                 [](const QHttpServerRequest& request) {
                     QJsonParseError parseError;
                     const auto document = QJsonDocument::fromJson(request.body(), &parseError);
                     if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                         return jsonResponse({
                                                 { QStringLiteral("code"), 1000 },
                                                 { QStringLiteral("message"), QStringLiteral("invalid request body") }
                                             },
                                             400);
                     }

                     const Services::UserService service;
                     return serviceResponse(service.uploadAvatar(authorizationHeader(request), document.object()));
                 });

    server.route(QStringLiteral("/uploads/avatars/<arg>"),
                 QHttpServerRequest::Method::Get,
                 [](const QString& fileName) {
                     return fileResponse(fileName);
                 });
}

} // namespace Backend::Routers
