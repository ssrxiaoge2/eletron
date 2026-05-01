#include "AuthClient.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/qscopeguard.h>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace {

constexpr char kLoginUrl[] = "http://127.0.0.1:8000/auth/login";

bool IsDatabaseErrorCode(const QString &code) {
  return code == "database_unavailable" || code == "database_error";
}

QString DatabaseErrorMessage() {
  return QStringLiteral("\u65e0\u6cd5\u8fde\u63a5\u6570\u636e\u5e93");
}

QString CredentialErrorMessage() {
  return QStringLiteral("\u8d26\u53f7\u6216\u5bc6\u7801\u9519\u8bef");
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
    emit loginFailed(DatabaseErrorMessage());
    return;
  }

  const int status_code =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const QByteArray response_body = reply->readAll();
  const QJsonDocument document = QJsonDocument::fromJson(response_body);
  const QJsonObject object = document.object();
  const QString code = object.value("code").toString();
  const bool success = object.value("success").toBool(false);

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
                        user.value("nickname").toString());
    return;
  }

  emit loginFailed(CredentialErrorMessage());
}
