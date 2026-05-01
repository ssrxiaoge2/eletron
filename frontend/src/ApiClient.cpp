#include "ApiClient.h"

#include <QtCore/QJsonDocument>
#include <QtCore/qscopeguard.h>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace {

constexpr char kBaseUrl[] = "http://127.0.0.1:8000";

}  // namespace

ApiClient *ApiClient::instance() {
  static auto *client = new ApiClient();
  return client;
}

ApiClient::ApiClient(QObject *parent) : QObject(parent) {}

void ApiClient::setToken(const QString &token) {
  token_ = token;
}

QString ApiClient::token() const {
  return token_;
}

void ApiClient::get(const QString &path, const QObject *receiver,
                    std::function<void(const QJsonObject &)> on_success,
                    std::function<void()> on_failure) {
  QNetworkReply *reply = network_manager_.get(CreateRequest(path));
  connect(reply, &QNetworkReply::finished, this, [=]() {
    HandleReply(reply, receiver, on_success, on_failure);
  });
}

void ApiClient::post(const QString &path, const QJsonObject &body,
                     const QObject *receiver,
                     std::function<void(const QJsonObject &)> on_success,
                     std::function<void()> on_failure) {
  QNetworkRequest request = CreateRequest(path);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  QNetworkReply *reply =
      network_manager_.post(request, QJsonDocument(body).toJson());
  connect(reply, &QNetworkReply::finished, this, [=]() {
    HandleReply(reply, receiver, on_success, on_failure);
  });
}

void ApiClient::put(const QString &path, const QJsonObject &body,
                    const QObject *receiver,
                    std::function<void(const QJsonObject &)> on_success,
                    std::function<void()> on_failure) {
  QNetworkRequest request = CreateRequest(path);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  QNetworkReply *reply =
      network_manager_.put(request, QJsonDocument(body).toJson());
  connect(reply, &QNetworkReply::finished, this, [=]() {
    HandleReply(reply, receiver, on_success, on_failure);
  });
}

QNetworkRequest ApiClient::CreateRequest(const QString &path) const {
  QNetworkRequest request(QUrl(QString::fromLatin1(kBaseUrl) + path));
  if (!token_.isEmpty()) {
    request.setRawHeader("Authorization",
                         ("Bearer " + token_).toUtf8());
  }
  return request;
}

void ApiClient::HandleReply(
    QNetworkReply *reply, const QObject *receiver,
    std::function<void(const QJsonObject &)> on_success,
    std::function<void()> on_failure) {
  const auto cleanup = qScopeGuard([reply]() { reply->deleteLater(); });
  if (receiver == nullptr) {
    return;
  }

  const QByteArray response_body = reply->readAll();
  const QJsonObject object = QJsonDocument::fromJson(response_body).object();
  const int code = object.value("code").toInt(-1);

  if (reply->error() == QNetworkReply::NoError && code == 0) {
    on_success(object);
    return;
  }

  if (on_failure) {
    on_failure();
  }
}
