#pragma once

#include <QtWidgets/QWidget>

class QLineEdit;
class QPushButton;

class LoginWindow : public QWidget {
  Q_OBJECT

 public:
  explicit LoginWindow(QWidget *parent = nullptr);

 signals:
  void loginSucceeded();

 private:
  void HandleMockLogin();

  QLineEdit *account_edit_;
  QLineEdit *password_edit_;
  QPushButton *login_button_;
};
