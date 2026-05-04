#pragma once

#include <QtCore/QObject>
#include <QtNetwork/QNetworkAccessManager>

class QNetworkReply;

class AuthClient : public QObject {
  Q_OBJECT

 public:
  explicit AuthClient(QObject *parent = nullptr);

  void Login(const QString &username, const QString &password);
  void QuickLogin(const QString &token);
  void Register(const QString &nickname, const QString &username,
                const QString &password);

 signals:
  void loginSucceeded(const QString &username, const QString &nickname,
                      const QString &token);
  void loginFailed(const QString &message);
  void quickLoginTokenExpired();
  void registerSucceeded(const QString &username, const QString &nickname,
                         const QString &token);
  void registerFailed(const QString &message);

 private:
  void HandleLoginReply(QNetworkReply *reply);
  void HandleQuickLoginReply(QNetworkReply *reply, const QString &token);
  void HandleRegisterReply(QNetworkReply *reply);

  QNetworkAccessManager network_manager_;
  QString pending_login_username_;
  QString pending_register_username_;
  QString pending_register_password_;
};
