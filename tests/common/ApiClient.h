#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QVariantMap>

struct ApiResponse {
    int httpStatus = 0;
    QJsonObject body;
    QByteArray rawBody;
    QString error;
    QString contentType;

    int code() const;
    QJsonValue data() const;
};

class ApiClient {
public:
    explicit ApiClient(QString baseUrl = QStringLiteral(TEST_BACKEND_BASE_URL));

    ApiResponse get(const QString& path, const QString& token = QString());
    ApiResponse post(const QString& path, const QJsonObject& payload, const QString& token = QString());
    ApiResponse put(const QString& path, const QJsonObject& payload, const QString& token = QString());
    ApiResponse deleteResource(const QString& path, const QString& token = QString());
    ApiResponse postMultipart(const QString& path,
                              const QString& token,
                              const QString& fileFieldName,
                              const QString& fileName,
                              const QByteArray& fileBytes,
                              const QString& mimeType,
                              const QVariantMap& fields);

private:
    ApiResponse send(const QString& method,
                     const QString& path,
                     const QJsonObject* payload,
                     const QString& token);

    ApiResponse sendRaw(const QString& method,
                        const QString& path,
                        const QByteArray& body,
                        const QString& contentType,
                        const QString& token);

    QString baseUrl_;
    QNetworkAccessManager manager_;
};
