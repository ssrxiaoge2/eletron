#include "LoginWindow.h"

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent) {
  setWindowTitle("QQ Login");
  setFixedSize(360, 240);

  auto *root_layout = new QVBoxLayout(this);
  auto *title_label = new QLabel("QQ Login", this);
  auto *form_layout = new QFormLayout();

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

  connect(login_button_, &QPushButton::clicked, this,
          &LoginWindow::HandleMockLogin);
}

void LoginWindow::HandleMockLogin() {
  // TODO(FE-Agent): Replace mock login after docs/api.md defines auth APIs.
  emit loginSucceeded();
}
