#include "GroupRouter.h"

#include "../services/GroupService.h"

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

QHttpServerResponse serviceResponse(const Services::GroupResult& result)
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

int queryInt(const QHttpServerRequest& request, const QString& name, int fallback)
{
    bool ok = false;
    const auto value = request.query().queryItemValue(name).toInt(&ok);
    return ok ? value : fallback;
}

} // namespace

void GroupRouter::registerRoutes(QHttpServer& server)
{
    server.route(QStringLiteral("/api/v1/groups"),
                 QHttpServerRequest::Method::Post,
                 [](const QHttpServerRequest& request) {
                     const auto parsed = parseJsonObject(request);
                     if (!parsed.ok) {
                         return jsonResponse(parsed.errorPayload, parsed.statusCode);
                     }
                     const Services::GroupService service;
                     return serviceResponse(service.create(authorizationHeader(request), parsed.body));
                 });

    server.route(QStringLiteral("/api/v1/groups"),
                 QHttpServerRequest::Method::Get,
                 [](const QHttpServerRequest& request) {
                     const Services::GroupService service;
                     return serviceResponse(service.listMine(authorizationHeader(request)));
                 });

    server.route(QStringLiteral("/api/v1/groups/<arg>"),
                 QHttpServerRequest::Method::Delete,
                 [](qint64 groupId, const QHttpServerRequest& request) {
                     const Services::GroupService service;
                     return serviceResponse(service.dissolve(authorizationHeader(request), groupId));
                 });

    server.route(QStringLiteral("/api/v1/groups/<arg>"),
                 QHttpServerRequest::Method::Get,
                 [](qint64 groupId, const QHttpServerRequest& request) {
                     const Services::GroupService service;
                     return serviceResponse(service.info(authorizationHeader(request), groupId));
                 });

    server.route(QStringLiteral("/api/v1/groups/<arg>/name"),
                 QHttpServerRequest::Method::Put,
                 [](qint64 groupId, const QHttpServerRequest& request) {
                     const auto parsed = parseJsonObject(request);
                     if (!parsed.ok) {
                         return jsonResponse(parsed.errorPayload, parsed.statusCode);
                     }
                     const Services::GroupService service;
                     return serviceResponse(service.updateName(authorizationHeader(request), groupId, parsed.body));
                 });

    server.route(QStringLiteral("/api/v1/groups/<arg>/announcement"),
                 QHttpServerRequest::Method::Put,
                 [](qint64 groupId, const QHttpServerRequest& request) {
                     const auto parsed = parseJsonObject(request);
                     if (!parsed.ok) {
                         return jsonResponse(parsed.errorPayload, parsed.statusCode);
                     }
                     const Services::GroupService service;
                     return serviceResponse(service.updateAnnouncement(authorizationHeader(request), groupId, parsed.body));
                 });

    server.route(QStringLiteral("/api/v1/groups/<arg>/members"),
                 QHttpServerRequest::Method::Get,
                 [](qint64 groupId, const QHttpServerRequest& request) {
                     const Services::GroupService service;
                     return serviceResponse(service.members(authorizationHeader(request), groupId));
                 });

    server.route(QStringLiteral("/api/v1/groups/<arg>/members/<arg>/role"),
                 QHttpServerRequest::Method::Put,
                 [](qint64 groupId, qint64 userId, const QHttpServerRequest& request) {
                     const auto parsed = parseJsonObject(request);
                     if (!parsed.ok) {
                         return jsonResponse(parsed.errorPayload, parsed.statusCode);
                     }
                     const Services::GroupService service;
                     return serviceResponse(service.setRole(authorizationHeader(request), groupId, userId, parsed.body));
                 });

    server.route(QStringLiteral("/api/v1/groups/<arg>/members/<arg>"),
                 QHttpServerRequest::Method::Delete,
                 [](qint64 groupId, qint64 userId, const QHttpServerRequest& request) {
                     const Services::GroupService service;
                     return serviceResponse(service.kick(authorizationHeader(request), groupId, userId));
                 });

    server.route(QStringLiteral("/api/v1/groups/<arg>/members/<arg>/mute"),
                 QHttpServerRequest::Method::Put,
                 [](qint64 groupId, qint64 userId, const QHttpServerRequest& request) {
                     const auto parsed = parseJsonObject(request);
                     if (!parsed.ok) {
                         return jsonResponse(parsed.errorPayload, parsed.statusCode);
                     }
                     const Services::GroupService service;
                     return serviceResponse(service.mute(authorizationHeader(request), groupId, userId, parsed.body));
                 });

    server.route(QStringLiteral("/api/v1/groups/<arg>/members/self"),
                 QHttpServerRequest::Method::Delete,
                 [](qint64 groupId, const QHttpServerRequest& request) {
                     const Services::GroupService service;
                     return serviceResponse(service.leave(authorizationHeader(request), groupId));
                 });

    server.route(QStringLiteral("/api/v1/groups/<arg>/messages"),
                 QHttpServerRequest::Method::Post,
                 [](qint64 groupId, const QHttpServerRequest& request) {
                     const auto parsed = parseJsonObject(request);
                     if (!parsed.ok) {
                         return jsonResponse(parsed.errorPayload, parsed.statusCode);
                     }
                     const Services::GroupService service;
                     return serviceResponse(service.sendMessage(authorizationHeader(request), groupId, parsed.body));
                 });

    server.route(QStringLiteral("/api/v1/groups/<arg>/messages"),
                 QHttpServerRequest::Method::Get,
                 [](qint64 groupId, const QHttpServerRequest& request) {
                     const Services::GroupService service;
                     return serviceResponse(service.history(
                         authorizationHeader(request),
                         groupId,
                         queryInt(request, QStringLiteral("page"), 1),
                         queryInt(request, QStringLiteral("pageSize"), 20)));
                 });
}

} // namespace Backend::Routers
