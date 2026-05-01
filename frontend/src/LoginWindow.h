#pragma once

#include <QtWidgets/QWidget>

class AuthClient;
class QComboBox;
class QLabel;
class QPushButton;
class QLineEdit;
class QStackedWidget;

class LoginWindow : public QWidget {
  Q_OBJECT

 public:
  explicit LoginWindow(QWidget *parent = nullptr);

 signals:
  void loginSucceeded();

 private:
  void HandleLogin();
  void HandleCachedLogin();
  void ShowPasswordLogin();
  void ShowCachedLogin();
  void HandleRegister();
  void LoadCachedUsers();
  void SaveCachedUser(const QString &username, const QString &nickname,
                      const QString &token);
  void SyncCachedUser(int index);
  void SyncPasswordUser(int index);
  void SetAuthPending(bool pending);
  QString CurrentUsername() const;
  QString CurrentCachedUsername() const;
  QString CachedToken(const QString &username) const;
  bool HasCachedUsers() const;

  AuthClient *auth_client_;
  QLabel *avatar_label_;
  QStackedWidget *mode_stack_;
  QComboBox *cached_account_combo_;
  QComboBox *username_combo_;
  QLineEdit *password_edit_;
  QPushButton *cached_login_button_;
  QPushButton *password_login_button_;
  QPushButton *cached_register_button_;
  QPushButton *register_button_;
  QPushButton *login_button_;
};
