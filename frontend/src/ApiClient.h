#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QLockFile>
#include <QtCore/QObject>
#include <QtNetwork/QNetworkAccessManager>

#include <functional>
#include <memory>

class QNetworkReply;

class ApiClient : public QObject {
  Q_OBJECT

 public:
  static ApiClient *instance();

  void setToken(const QString &token);
  QString token() const;
  QString lockedUsername() const;
  bool acquireUserSessionLock(const QString &username);
  void releaseUserSessionLock();

  void get(const QString &path, const QObject *receiver,
           std::function<void(const QJsonObject &)> on_success,
           std::function<void()> on_failure = {});
  void post(const QString &path, const QJsonObject &body,
            const QObject *receiver,
            std::function<void(const QJsonObject &)> on_success,
            std::function<void()> on_failure = {});
  bool postBlocking(const QString &path, const QJsonObject &body,
                    int timeout_ms);
  void put(const QString &path, const QJsonObject &body, const QObject *receiver,
           std::function<void(const QJsonObject &)> on_success,
           std::function<void()> on_failure = {});
  void deleteResource(const QString &path, const QObject *receiver,
                      std::function<void(const QJsonObject &)> on_success,
                      std::function<void()> on_failure = {});
  QNetworkReply *uploadFile(
      const QString &file_path, int type, int receiver_id,
      const QObject *receiver,
      std::function<void(const QJsonObject &)> on_success,
      std::function<void()> on_failure = {},
      std::function<void(qint64, qint64)> on_progress = {});
  QNetworkReply *downloadBytes(
      const QString &path, const QObject *receiver,
      std::function<void(const QByteArray &)> on_success,
      std::function<void()> on_failure = {},
      std::function<void(qint64, qint64)> on_progress = {});

 private:
  explicit ApiClient(QObject *parent = nullptr);

  QNetworkRequest CreateRequest(const QString &path) const;
  void HandleReply(QNetworkReply *reply, const QObject *receiver,
                   std::function<void(const QJsonObject &)> on_success,
                   std::function<void()> on_failure);

  QString token_;
  QString locked_username_;
  std::unique_ptr<QLockFile> user_session_lock_;
  QNetworkAccessManager network_manager_;
};
