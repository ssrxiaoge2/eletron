#include "FileRouter.h"

#include "../services/FileService.h"

#include <QHttpHeaders>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <utility>

namespace Backend::Routers {
namespace {

struct UploadForm {
    bool ok = false;
    Services::UploadedFilePart file;
    int type = 0;
    qint64 receiverId = 0;
    qint64 groupId = 0;
};

QString authorizationHeader(const QHttpServerRequest& request)
{
    auto value = request.value(QByteArrayLiteral("Authorization"));
    if (value.isEmpty()) {
        value = request.value(QByteArrayLiteral("authorization"));
    }
    return QString::fromUtf8(value);
}

QString contentTypeHeader(const QHttpServerRequest& request)
{
    auto value = request.value(QByteArrayLiteral("Content-Type"));
    if (value.isEmpty()) {
        value = request.value(QByteArrayLiteral("content-type"));
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

QHttpServerResponse serviceResponse(const Services::FileResult& result)
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

QHttpServerResponse binaryErrorResponse(const Services::FileBinaryResult& result)
{
    return jsonResponse({
                            { QStringLiteral("code"), result.code },
                            { QStringLiteral("message"), result.message }
                        },
                        result.httpStatus);
}

QString dispositionValue(const QString& contentDisposition, const QString& name)
{
    const QRegularExpression expression(QStringLiteral("%1=(?:\"([^\"]*)\"|([^;\\r\\n]*))").arg(name));
    const auto match = expression.match(contentDisposition);
    if (!match.hasMatch()) {
        return {};
    }
    const auto quoted = match.captured(1);
    return quoted.isEmpty() ? match.captured(2).trimmed() : quoted;
}

QByteArray parseBoundary(const QString& contentType)
{
    const QRegularExpression expression(QStringLiteral("boundary=\"?([^\";]+)\"?"));
    const auto match = expression.match(contentType);
    return match.hasMatch() ? match.captured(1).toUtf8() : QByteArray();
}

UploadForm parseMultipart(const QHttpServerRequest& request)
{
    UploadForm form;
    const auto boundary = parseBoundary(contentTypeHeader(request));
    if (boundary.isEmpty()) {
        return form;
    }

    const auto body = request.body();
    const auto delimiter = QByteArray("--") + boundary;
    QList<QByteArray> sections;
    qsizetype start = 0;
    while (true) {
        const auto index = body.indexOf(delimiter, start);
        if (index < 0) {
            sections.append(body.mid(start));
            break;
        }
        sections.append(body.mid(start, index - start));
        start = index + delimiter.size();
    }
    for (auto section : sections) {
        if (section.startsWith("--") || section.trimmed().isEmpty()) {
            continue;
        }
        if (section.startsWith("\r\n")) {
            section.remove(0, 2);
        }
        if (section.endsWith("\r\n")) {
            section.chop(2);
        }

        const auto headerEnd = section.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            continue;
        }
        const auto headerBytes = section.left(headerEnd);
        const auto data = section.mid(headerEnd + 4);
        const auto headerLines = QString::fromUtf8(headerBytes).split(QStringLiteral("\r\n"));

        QString partName;
        QString fileName;
        QString mimeType;
        for (const auto& line : headerLines) {
            const auto colon = line.indexOf(QLatin1Char(':'));
            if (colon < 0) {
                continue;
            }
            const auto key = line.left(colon).trimmed();
            const auto value = line.mid(colon + 1).trimmed();
            if (key.compare(QStringLiteral("Content-Disposition"), Qt::CaseInsensitive) == 0) {
                partName = dispositionValue(value, QStringLiteral("name"));
                fileName = dispositionValue(value, QStringLiteral("filename"));
            } else if (key.compare(QStringLiteral("Content-Type"), Qt::CaseInsensitive) == 0) {
                mimeType = value;
            }
        }

        if (partName == QStringLiteral("file")) {
            form.file.fileName = fileName;
            form.file.mimeType = mimeType;
            form.file.bytes = data;
        } else if (partName == QStringLiteral("type")) {
            form.type = QString::fromUtf8(data).trimmed().toInt();
        } else if (partName == QStringLiteral("receiverId")) {
            form.receiverId = QString::fromUtf8(data).trimmed().toLongLong();
        } else if (partName == QStringLiteral("groupId")) {
            form.groupId = QString::fromUtf8(data).trimmed().toLongLong();
        }
    }

    form.ok = !form.file.bytes.isEmpty()
        && !form.file.fileName.trimmed().isEmpty()
        && (form.type == 1 || form.type == 2)
        && (form.groupId > 0 || form.receiverId > 0);
    return form;
}

QHttpServerResponse fileResponse(const Services::FileBinaryResult& result, bool attachment)
{
    if (result.code != 0) {
        return binaryErrorResponse(result);
    }

    QHttpServerResponse response(
        result.mimeType.toUtf8(),
        result.data,
        QHttpServerResponse::StatusCode::Ok);

    QHttpHeaders headers = response.headers();
    headers.replaceOrAppend(QHttpHeaders::WellKnownHeader::ContentLength, QByteArray::number(result.fileSize));
    if (attachment) {
        auto safeName = result.fileName;
        safeName.replace(QLatin1Char('"'), QLatin1Char('_'));
        headers.replaceOrAppend(
            QHttpHeaders::WellKnownHeader::ContentDisposition,
            QStringLiteral("attachment; filename=\"%1\"").arg(safeName));
    }
    response.setHeaders(std::move(headers));
    return response;
}

} // namespace

void FileRouter::registerRoutes(QHttpServer& server)
{
    server.route(QStringLiteral("/api/v1/files/upload"),
                 QHttpServerRequest::Method::Post,
                 [](const QHttpServerRequest& request) {
                     const auto form = parseMultipart(request);
                     if (!form.ok) {
                         return jsonResponse({
                                                 { QStringLiteral("code"), 1000 },
                                                 { QStringLiteral("message"), QStringLiteral("invalid upload payload") }
                                             },
                                             400);
                     }

                     const Services::FileService service;
                     return serviceResponse(service.upload(
                         authorizationHeader(request),
                         form.file,
                         form.type,
                         form.receiverId,
                         form.groupId));
                 });

    server.route(QStringLiteral("/api/v1/files/download/<arg>"),
                 QHttpServerRequest::Method::Get,
                 [](qint64 fileId, const QHttpServerRequest& request) {
                     const Services::FileService service;
                     return fileResponse(service.download(authorizationHeader(request), fileId), true);
                 });

    server.route(QStringLiteral("/api/v1/files/thumbnail/<arg>"),
                 QHttpServerRequest::Method::Get,
                 [](qint64 fileId, const QHttpServerRequest& request) {
                     const Services::FileService service;
                     return fileResponse(service.thumbnail(authorizationHeader(request), fileId), false);
                 });

    server.route(QStringLiteral("/api/v1/files/info/<arg>"),
                 QHttpServerRequest::Method::Get,
                 [](qint64 fileId, const QHttpServerRequest& request) {
                     const Services::FileService service;
                     return serviceResponse(service.info(authorizationHeader(request), fileId));
                 });
}

} // namespace Backend::Routers
