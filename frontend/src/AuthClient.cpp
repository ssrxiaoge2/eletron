#include "AuthClient.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/qscopeguard.h>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace {

constexpr char kLoginUrl[] = "http://127.0.0.1:8000/auth/login";
constexpr char kRegisterUrl[] = "http://127.0.0.1:8000/auth/register";
constexpr char kQuickLoginUrl[] = "http://127.0.0.1:8000/auth/quick-login";

bool IsDatabaseErrorCode(const QString &code) {
  return code == "database_unavailable" || code == "database_error";
}

QString DatabaseErrorMessage() {
  return QStringLiteral("\u65e0\u6cd5\u8fde\u63a5\u6570\u636e\u5e93");
}

QString CredentialErrorMessage() {
  return QStringLiteral("\u8d26\u53f7\u6216\u5bc6\u7801\u9519\u8bef");
}

QString SessionErrorMessage() {
  return QStringLiteral(
      "\u767b\u5f55\u72b6\u6001\u5df2\u8fc7\u671f\uff0c\u8bf7\u4f7f"
      "\u7528\u8d26\u53f7\u5bc6\u7801\u767b\u5f55");
}

QString AccountExistsMessage() {
  return QStringLiteral("\u8d26\u53f7\u5df2\u5b58\u5728");
}

QString RegisterFailedMessage() {
  return QStringLiteral("\u6ce8\u518c\u5931\u8d25");
}

}  // namespace

AuthClient::AuthClient(QObject *parent) : QObject(parent) {}

QString ExtractSessionToken(const QJsonObject &object) {
  return object.value("session").toObject().value("token").toString();
}

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

void AuthClient::QuickLogin(const QString &token) {
  QNetworkRequest request{QUrl(kQuickLoginUrl)};
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QJsonObject payload;
  payload.insert("token", token);

  QNetworkReply *reply =
      network_manager_.post(request, QJsonDocument(payload).toJson());
  connect(reply, &QNetworkReply::finished, this,
          [this, reply]() { HandleQuickLoginReply(reply); });
}

void AuthClient::Register(const QString &nickname, const QString &username,
                          const QString &password) {
  QNetworkRequest request{QUrl(kRegisterUrl)};
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QJsonObject payload;
  payload.insert("username", username);
  payload.insert("password", password);
  if (!nickname.isEmpty()) {
    payload.insert("nickname", nickname);
  }

  QNetworkReply *reply =
      network_manager_.post(request, QJsonDocument(payload).toJson());
  connect(reply, &QNetworkReply::finished, this,
          [this, reply]() { HandleRegisterReply(reply); });
}

void AuthClient::HandleLoginReply(QNetworkReply *reply) {
  const auto cleanup = qScopeGuard([reply]() { reply->deleteLater(); });

  const int status_code =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const QByteArray response_body = reply->readAll();
  const QJsonDocument document = QJsonDocument::fromJson(response_body);
  const QJsonObject object = document.object();
  const QString code = object.value("code").toString();
  const bool success = object.value("success").toBool(false);

  if (reply->error() != QNetworkReply::NoError && status_code == 0) {
    emit loginFailed(DatabaseErrorMessage());
    return;
  }

  if (status_code == 503 || IsDatabaseErrorCode(code)) {
    emit loginFailed(DatabaseErrorMessage());
    return;
  }

  if (status_code == 401 || status_code == 403 || status_code == 400 ||
      (!success && status_code >= 200 && status_code < 300)) {
    emit loginFailed(CredentialErrorMessage());
    return;
  }

  if (status_code >= 200 && status_code < 300 && success) {
    const QJsonObject user = object.value("user").toObject();
    emit loginSucceeded(user.value("account").toString(),
                        user.value("nickname").toString(),
                        ExtractSessionToken(object));
    return;
  }

  emit loginFailed(CredentialErrorMessage());
}

void AuthClient::HandleQuickLoginReply(QNetworkReply *reply) {
  const auto cleanup = qScopeGuard([reply]() { reply->deleteLater(); });

  const int status_code =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const QByteArray response_body = reply->readAll();
  const QJsonDocument document = QJsonDocument::fromJson(response_body);
  const QJsonObject object = document.object();
  const QString code = object.value("code").toString();
  const bool success = object.value("success").toBool(false);

  if (reply->error() != QNetworkReply::NoError && status_code == 0) {
    emit loginFailed(DatabaseErrorMessage());
    return;
  }

  if (status_code == 503 || IsDatabaseErrorCode(code)) {
    emit loginFailed(DatabaseErrorMessage());
    return;
  }

  if (status_code == 401 || code == "invalid_session" ||
      (!success && status_code >= 200 && status_code < 300)) {
    emit loginFailed(SessionErrorMessage());
    return;
  }

  if (status_code >= 200 && status_code < 300 && success) {
    const QJsonObject user = object.value("user").toObject();
    emit loginSucceeded(user.value("account").toString(),
                        user.value("nickname").toString(),
                        ExtractSessionToken(object));
    return;
  }

  emit loginFailed(SessionErrorMessage());
}

void AuthClient::HandleRegisterReply(QNetworkReply *reply) {
  const auto cleanup = qScopeGuard([reply]() { reply->deleteLater(); });

  const int status_code =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const QByteArray response_body = reply->readAll();
  const QJsonDocument document = QJsonDocument::fromJson(response_body);
  const QJsonObject object = document.object();
  const QString code = object.value("code").toString();
  const bool success = object.value("success").toBool(false);

  if (reply->error() != QNetworkReply::NoError && status_code == 0) {
    emit registerFailed(DatabaseErrorMessage());
    return;
  }

  if (status_code == 503 || IsDatabaseErrorCode(code)) {
    emit registerFailed(DatabaseErrorMessage());
    return;
  }

  if (status_code == 409 || code == "account_exists") {
    emit registerFailed(AccountExistsMessage());
    return;
  }

  if (status_code == 400 || (!success && status_code >= 200 &&
                             status_code < 300)) {
    emit registerFailed(RegisterFailedMessage());
    return;
  }

  if (status_code >= 200 && status_code < 300 && success) {
    const QJsonObject user = object.value("user").toObject();
    emit registerSucceeded(user.value("account").toString(),
                           user.value("nickname").toString(),
                           ExtractSessionToken(object));
    return;
  }

  emit registerFailed(RegisterFailedMessage());
}
