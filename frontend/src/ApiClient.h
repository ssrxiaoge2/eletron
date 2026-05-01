#pragma once

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtNetwork/QNetworkAccessManager>

class QNetworkReply;

class ApiClient : public QObject {
  Q_OBJECT

 public:
  static ApiClient *instance();

  void setToken(const QString &token);
  QString token() const;

  void get(const QString &path, const QObject *receiver,
           std::function<void(const QJsonObject &)> on_success,
           std::function<void()> on_failure = {});
  void post(const QString &path, const QJsonObject &body,
            const QObject *receiver,
            std::function<void(const QJsonObject &)> on_success,
            std::function<void()> on_failure = {});

 private:
  explicit ApiClient(QObject *parent = nullptr);

  QNetworkRequest CreateRequest(const QString &path) const;
  void HandleReply(QNetworkReply *reply, const QObject *receiver,
                   std::function<void(const QJsonObject &)> on_success,
                   std::function<void()> on_failure);

  QString token_;
  QNetworkAccessManager network_manager_;
};
