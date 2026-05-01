#include "AuthClient.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/qscopeguard.h>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace {

constexpr char kLoginUrl[] = "http://127.0.0.1:8000/auth/login";
constexpr char kDatabaseErrorMessage[] = "无法连接数据库";
constexpr char kCredentialErrorMessage[] = "账号或密码错误";
constexpr char kPendingApiMessage[] = "登录接口待对接";

bool IsDatabaseErrorCode(const QString &code) {
  return code == "database_unavailable" || code == "database_error" ||
         code == "db_unavailable" || code == "db_error";
}

}  // namespace

AuthClient::AuthClient(QObject *parent) : QObject(parent) {}

void AuthClient::Login(const QString &username, const QString &password) {
  QNetworkRequest request{QUrl(kLoginUrl)};
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QJsonObject payload;
  payload.insert("username", username);
  payload.insert("password", password);

  QNetworkReply *reply =
      network_manager_.post(request, QJsonDocument(payload).toJson());
  connect(reply, &QNetworkReply::finished, this,
          [this, reply]() { HandleLoginReply(reply); });
}

void AuthClient::HandleLoginReply(QNetworkReply *reply) {
  const auto cleanup = qScopeGuard([reply]() { reply->deleteLater(); });

  if (reply->error() != QNetworkReply::NoError) {
    emit loginFailed(kDatabaseErrorMessage);
    return;
  }

  const int status_code =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const QByteArray response_body = reply->readAll();
  const QJsonDocument document = QJsonDocument::fromJson(response_body);
  const QJsonObject object = document.object();
  const QString code = object.value("code").toString();
  const bool success = object.value("success").toBool(false);

  if (status_code == 404) {
    emit loginFailed(kPendingApiMessage);
    return;
  }

  if (status_code == 503 || IsDatabaseErrorCode(code)) {
    emit loginFailed(kDatabaseErrorMessage);
    return;
  }

  if (status_code == 401 || status_code == 403 || status_code == 400 ||
      (!success && status_code >= 200 && status_code < 300)) {
    emit loginFailed(kCredentialErrorMessage);
    return;
  }

  if (status_code >= 200 && status_code < 300) {
    emit loginSucceeded();
    return;
  }

  emit loginFailed(kCredentialErrorMessage);
}
