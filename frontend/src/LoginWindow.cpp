#include "LoginWindow.h"

#include "AuthClient.h"

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent) {
  setWindowTitle("QQ Login");
  setFixedSize(360, 240);

  auto *root_layout = new QVBoxLayout(this);
  auto *title_label = new QLabel("QQ Login", this);
  auto *form_layout = new QFormLayout();

  auth_client_ = new AuthClient(this);
  account_edit_ = new QLineEdit(this);
  password_edit_ = new QLineEdit(this);
  login_button_ = new QPushButton("Login", this);

  title_label->setAlignment(Qt::AlignCenter);
  account_edit_->setPlaceholderText("Mock account");
  password_edit_->setPlaceholderText("Mock password");
  password_edit_->setEchoMode(QLineEdit::Password);

  form_layout->addRow("Account", account_edit_);
  form_layout->addRow("Password", password_edit_);

  root_layout->addWidget(title_label);
  root_layout->addLayout(form_layout);
  root_layout->addWidget(login_button_);

  connect(login_button_, &QPushButton::clicked, this, &LoginWindow::HandleLogin);
  connect(auth_client_, &AuthClient::loginSucceeded, this, [this]() {
    SetLoginPending(false);
    emit loginSucceeded();
  });
  connect(auth_client_, &AuthClient::loginFailed, this,
          [this](const QString &message) {
            SetLoginPending(false);
            QMessageBox::warning(this, "Login Failed", message);
          });
}

void LoginWindow::HandleLogin() {
  const QString username = account_edit_->text().trimmed();
  const QString password = password_edit_->text();

  if (username.isEmpty() || password.isEmpty()) {
    QMessageBox::warning(this, "Login Failed", "请输入账号和密码");
    return;
  }

  SetLoginPending(true);
  auth_client_->Login(username, password);
}

void LoginWindow::SetLoginPending(bool pending) {
  account_edit_->setEnabled(!pending);
  password_edit_->setEnabled(!pending);
  login_button_->setEnabled(!pending);
  login_button_->setText(pending ? "Logging in..." : "Login");
}
