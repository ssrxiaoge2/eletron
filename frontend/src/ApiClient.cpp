#include "ApiClient.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QJsonDocument>
#include <QtCore/QLockFile>
#include <QtCore/QTimer>
#include <QtCore/qscopeguard.h>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace {

constexpr char kBaseUrl[] = "http://127.0.0.1:8000";
constexpr char kLockFilePrefix[] = "feige-im-user-";

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

QString ApiClient::lockedUsername() const {
  return locked_username_;
}

bool ApiClient::acquireUserSessionLock(const QString &username) {
  if (username.isEmpty()) {
    return false;
  }
  if (user_session_lock_ != nullptr && locked_username_ == username &&
      user_session_lock_->isLocked()) {
    return true;
  }

  releaseUserSessionLock();
  const QByteArray key =
      QCryptographicHash::hash(username.toUtf8(), QCryptographicHash::Sha256)
          .toHex();
  const QString lock_path = QDir::temp().filePath(
      QString::fromLatin1(kLockFilePrefix) + QString::fromLatin1(key) +
      ".lock");
  auto lock = std::make_unique<QLockFile>(lock_path);
  lock->setStaleLockTime(30000);
  if (!lock->tryLock(0)) {
    return false;
  }

  locked_username_ = username;
  user_session_lock_ = std::move(lock);
  return true;
}

void ApiClient::releaseUserSessionLock() {
  if (user_session_lock_ != nullptr) {
    user_session_lock_->unlock();
    user_session_lock_.reset();
  }
  locked_username_.clear();
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

bool ApiClient::postBlocking(const QString &path, const QJsonObject &body,
                             int timeout_ms) {
  QNetworkRequest request = CreateRequest(path);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  QNetworkReply *reply =
      network_manager_.post(request, QJsonDocument(body).toJson());

  QEventLoop loop;
  QTimer timer;
  bool timed_out = false;
  timer.setSingleShot(true);

  connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  connect(&timer, &QTimer::timeout, &loop, [&]() {
    timed_out = true;
    reply->abort();
    loop.quit();
  });

  timer.start(timeout_ms);
  loop.exec();
  if (timer.isActive()) {
    timer.stop();
  }

  const QByteArray response_body = reply->readAll();
  const QJsonObject object = QJsonDocument::fromJson(response_body).object();
  const int code = object.value("code").toInt(-1);
  const bool ok = !timed_out && reply->error() == QNetworkReply::NoError &&
                  code == 0;
  reply->deleteLater();
  return ok;
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
