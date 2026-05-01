#pragma once

#include <QtCore/QObject>
#include <QtNetwork/QNetworkAccessManager>

class QNetworkReply;

class AuthClient : public QObject {
  Q_OBJECT

 public:
  explicit AuthClient(QObject *parent = nullptr);

  void Login(const QString &username, const QString &password);

 signals:
  void loginSucceeded();
  void loginFailed(const QString &message);

 private:
  void HandleLoginReply(QNetworkReply *reply);

  QNetworkAccessManager network_manager_;
};
