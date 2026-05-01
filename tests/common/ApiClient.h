#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QString>

struct ApiResponse {
    int httpStatus = 0;
    QJsonObject body;
    QByteArray rawBody;
    QString error;

    int code() const;
    QJsonValue data() const;
};

class ApiClient {
public:
    explicit ApiClient(QString baseUrl = QStringLiteral(TEST_BACKEND_BASE_URL));

    ApiResponse get(const QString& path, const QString& token = QString());
    ApiResponse post(const QString& path, const QJsonObject& payload, const QString& token = QString());
    ApiResponse put(const QString& path, const QJsonObject& payload, const QString& token = QString());

private:
    ApiResponse send(const QString& method,
                     const QString& path,
                     const QJsonObject* payload,
                     const QString& token);

    QString baseUrl_;
    QNetworkAccessManager manager_;
};
