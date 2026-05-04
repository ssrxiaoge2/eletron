#include "AuthRouter.h"

#include "../services/AuthService.h"

#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>

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

QHttpServerResponse serviceResponse(const Backend::Services::AuthResult& result)
{
    QJsonObject payload {
        { QStringLiteral("code"), result.code },
        { QStringLiteral("message"), result.message }
    };
    if (!result.data.isEmpty()) {
        payload.insert(QStringLiteral("data"), result.data);
    }
    return jsonResponse(payload, result.httpStatus);
}

struct CredentialsResult {
    bool ok = false;
    QString username;
    QString password;
    QJsonObject errorPayload;
    int statusCode = 400;
};

CredentialsResult parseCredentials(const QHttpServerRequest& request)
{
    CredentialsResult result;
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(request.body(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        result.errorPayload = {
            { QStringLiteral("code"), 1000 },
            { QStringLiteral("message"), QStringLiteral("invalid request body") }
        };
        return result;
    }

    const auto body = document.object();
    result.username = body.value(QStringLiteral("username")).toString().trimmed();
    result.password = body.value(QStringLiteral("password")).toString();
    if (result.username.isEmpty() || result.password.isEmpty()) {
        result.errorPayload = {
            { QStringLiteral("code"), 1000 },
            { QStringLiteral("message"), QStringLiteral("username and password are required") }
        };
        return result;
    }

    if (result.username.size() > 32 || result.password.size() > 128) {
        result.errorPayload = {
            { QStringLiteral("code"), 1000 },
            { QStringLiteral("message"), QStringLiteral("username or password is too long") }
        };
        return result;
    }

    result.ok = true;
    return result;
}

} // namespace

void AuthRouter::registerRoutes(QHttpServer& server)
{
    server.route(QStringLiteral("/api/v1/auth/register"),
                 QHttpServerRequest::Method::Post,
                 [](const QHttpServerRequest& request) {
                     const auto credentials = parseCredentials(request);
                     if (!credentials.ok) {
                         return jsonResponse(credentials.errorPayload, credentials.statusCode);
                     }

                     const Backend::Services::AuthService service;
                     return serviceResponse(service.registerUser(credentials.username, credentials.password));
                 });

    server.route(QStringLiteral("/api/v1/auth/login"),
                 QHttpServerRequest::Method::Post,
                 [](const QHttpServerRequest& request) {
                     const auto credentials = parseCredentials(request);
                     if (!credentials.ok) {
                         return jsonResponse(credentials.errorPayload, credentials.statusCode);
                     }

                     const Backend::Services::AuthService service;
                     return serviceResponse(service.login(credentials.username, credentials.password));
                 });

    server.route(QStringLiteral("/api/v1/auth/logout"),
                 QHttpServerRequest::Method::Post,
                 [](const QHttpServerRequest& request) {
                     const Backend::Services::AuthService service;
                     return serviceResponse(service.logout(authorizationHeader(request)));
                 });

    server.route(QStringLiteral("/api/v1/auth/offline"),
                 QHttpServerRequest::Method::Post,
                 [](const QHttpServerRequest& request) {
                     const Backend::Services::AuthService service;
                     return serviceResponse(service.offline(authorizationHeader(request)));
                 });
}

} // namespace Backend::Routers
