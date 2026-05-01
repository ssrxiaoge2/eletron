#pragma once

#include <QtWidgets/QWidget>

class AuthClient;
class QComboBox;
class QPushButton;
class QLineEdit;

class LoginWindow : public QWidget {
  Q_OBJECT

 public:
  explicit LoginWindow(QWidget *parent = nullptr);

 signals:
  void loginSucceeded();

 private:
  void HandleLogin();
  void HandleRegister();
  void LoadCachedUsers();
  void SaveCachedUser(const QString &username, const QString &nickname);
  void SyncNicknameFromCachedUser(int index);
  void SetAuthPending(bool pending);
  QString CurrentUsername() const;

  AuthClient *auth_client_;
  QLineEdit *nickname_edit_;
  QComboBox *username_combo_;
  QLineEdit *password_edit_;
  QPushButton *register_button_;
  QPushButton *login_button_;
};
