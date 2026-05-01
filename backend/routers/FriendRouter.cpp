#include "FriendRouter.h"

#include "../services/FriendService.h"

#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QVariant>

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

QHttpServerResponse serviceResponse(const Services::FriendResult& result)
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

struct ParseResult {
    bool ok = false;
    QJsonObject body;
    QJsonObject errorPayload;
    int statusCode = 400;
};

ParseResult parseJsonObject(const QHttpServerRequest& request)
{
    ParseResult result;
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(request.body(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        result.errorPayload = {
            { QStringLiteral("code"), 1000 },
            { QStringLiteral("message"), QStringLiteral("invalid request body") }
        };
        return result;
    }

    result.ok = true;
    result.body = document.object();
    return result;
}

} // namespace

void FriendRouter::registerRoutes(QHttpServer& server)
{
    server.route(QStringLiteral("/api/v1/users/search"),
                 QHttpServerRequest::Method::Get,
                 [](const QHttpServerRequest& request) {
                     const auto keyword = request.query().queryItemValue(QStringLiteral("keyword"));
                     const Services::FriendService service;
                     return serviceResponse(service.searchUsers(authorizationHeader(request), keyword));
                 });

    server.route(QStringLiteral("/api/v1/friends/apply"),
                 QHttpServerRequest::Method::Post,
                 [](const QHttpServerRequest& request) {
                     const auto parsed = parseJsonObject(request);
                     if (!parsed.ok) {
                         return jsonResponse(parsed.errorPayload, parsed.statusCode);
                     }

                     const auto targetUserId = parsed.body.value(QStringLiteral("targetUserId")).toVariant().toLongLong();
                     const Services::FriendService service;
                     return serviceResponse(service.apply(authorizationHeader(request), targetUserId));
                 });

    server.route(QStringLiteral("/api/v1/friends/requests"),
                 QHttpServerRequest::Method::Get,
                 [](const QHttpServerRequest& request) {
                     const Services::FriendService service;
                     return serviceResponse(service.requests(authorizationHeader(request)));
                 });

    server.route(QStringLiteral("/api/v1/friends/handle"),
                 QHttpServerRequest::Method::Post,
                 [](const QHttpServerRequest& request) {
                     const auto parsed = parseJsonObject(request);
                     if (!parsed.ok) {
                         return jsonResponse(parsed.errorPayload, parsed.statusCode);
                     }

                     const auto requestId = parsed.body.value(QStringLiteral("requestId")).toVariant().toLongLong();
                     const auto action = parsed.body.value(QStringLiteral("action")).toInt();
                     const Services::FriendService service;
                     return serviceResponse(service.handle(authorizationHeader(request), requestId, action));
                 });

    server.route(QStringLiteral("/api/v1/friends"),
                 QHttpServerRequest::Method::Get,
                 [](const QHttpServerRequest& request) {
                     const Services::FriendService service;
                     return serviceResponse(service.friends(authorizationHeader(request)));
                 });

    server.route(QStringLiteral("/api/v1/conversations"),
                 QHttpServerRequest::Method::Post,
                 [](const QHttpServerRequest& request) {
                     const auto parsed = parseJsonObject(request);
                     if (!parsed.ok) {
                         return jsonResponse(parsed.errorPayload, parsed.statusCode);
                     }

                     const auto targetUserId = parsed.body.value(QStringLiteral("targetUserId")).toVariant().toLongLong();
                     const Services::FriendService service;
                     return serviceResponse(service.startConversation(authorizationHeader(request), targetUserId));
                 });
}

} // namespace Backend::Routers
