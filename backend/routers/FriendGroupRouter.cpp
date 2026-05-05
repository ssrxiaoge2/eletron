#include "FriendGroupRouter.h"

#include "../services/FriendGroupService.h"

#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
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

QHttpServerResponse serviceResponse(const Services::FriendGroupResult& result)
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

void FriendGroupRouter::registerRoutes(QHttpServer& server)
{
    server.route(QStringLiteral("/api/v1/friend-groups"),
                 QHttpServerRequest::Method::Get,
                 [](const QHttpServerRequest& request) {
                     const Services::FriendGroupService service;
                     return serviceResponse(service.list(authorizationHeader(request)));
                 });

    server.route(QStringLiteral("/api/v1/friend-groups"),
                 QHttpServerRequest::Method::Post,
                 [](const QHttpServerRequest& request) {
                     const auto parsed = parseJsonObject(request);
                     if (!parsed.ok) {
                         return jsonResponse(parsed.errorPayload, parsed.statusCode);
                     }
                     const Services::FriendGroupService service;
                     return serviceResponse(service.create(authorizationHeader(request), parsed.body));
                 });

    server.route(QStringLiteral("/api/v1/friend-groups/<arg>"),
                 QHttpServerRequest::Method::Put,
                 [](qint64 groupId, const QHttpServerRequest& request) {
                     const auto parsed = parseJsonObject(request);
                     if (!parsed.ok) {
                         return jsonResponse(parsed.errorPayload, parsed.statusCode);
                     }
                     const Services::FriendGroupService service;
                     return serviceResponse(service.rename(authorizationHeader(request), groupId, parsed.body));
                 });

    server.route(QStringLiteral("/api/v1/friend-groups/<arg>"),
                 QHttpServerRequest::Method::Delete,
                 [](qint64 groupId, const QHttpServerRequest& request) {
                     const Services::FriendGroupService service;
                     return serviceResponse(service.remove(authorizationHeader(request), groupId));
                 });

    server.route(QStringLiteral("/api/v1/friends/<arg>/group"),
                 QHttpServerRequest::Method::Put,
                 [](qint64 friendUserId, const QHttpServerRequest& request) {
                     const auto parsed = parseJsonObject(request);
                     if (!parsed.ok) {
                         return jsonResponse(parsed.errorPayload, parsed.statusCode);
                     }
                     const Services::FriendGroupService service;
                     return serviceResponse(service.moveFriend(authorizationHeader(request), friendUserId, parsed.body));
                 });
}

} // namespace Backend::Routers
