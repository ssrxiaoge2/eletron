#include "LoginWindow.h"

#include "AuthClient.h"

#include <QtCore/QSettings>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace {

constexpr char kSettingsOrg[] = "Feige";
constexpr char kSettingsApp[] = "DesktopIM";
constexpr char kCachedUsersGroup[] = "login_users";

QString TextBrandName() {
  return QStringLiteral("\u98de\u9e3d\u901a\u8baf");
}

QString TextNickname() {
  return QStringLiteral("\u6635\u79f0");
}

QString TextUsername() {
  return QStringLiteral("\u7528\u6237\u540d");
}

QString TextPassword() {
  return QStringLiteral("\u5bc6\u7801");
}

QString TextRegister() {
  return QStringLiteral("\u6ce8\u518c");
}

QString TextLogin() {
  return QStringLiteral("\u767b\u5f55");
}

QString TextLoginPending() {
  return QStringLiteral("\u767b\u5f55\u4e2d...");
}

QString TextRegisterPending() {
  return QStringLiteral("\u6ce8\u518c\u4e2d...");
}

QString TextLoginFailed() {
  return QStringLiteral("\u767b\u5f55\u5931\u8d25");
}

QString TextRegisterTitle() {
  return QStringLiteral("\u6ce8\u518c");
}

QString TextRegisterSucceeded() {
  return QStringLiteral("\u6ce8\u518c\u6210\u529f");
}

QString TextMissingLoginFields() {
  return QStringLiteral("\u8bf7\u8f93\u5165\u7528\u6237\u540d\u548c\u5bc6\u7801");
}

QString TextMissingRegisterFields() {
  return QStringLiteral(
      "\u8bf7\u8f93\u5165\u6635\u79f0\u3001\u7528\u6237\u540d\u548c\u5bc6"
      "\u7801");
}

QString TextUsernamePlaceholder() {
  return QStringLiteral("\u8f93\u5165\u6216\u9009\u62e9\u7528\u6237\u540d");
}

QString TextNicknamePlaceholder() {
  return QStringLiteral("\u8f93\u5165\u6635\u79f0");
}

QString TextPasswordPlaceholder() {
  return QStringLiteral("\u8f93\u5165\u5bc6\u7801");
}

QString LoginWindowStyle() {
  return R"(
    LoginWindow {
      background: #191919;
      color: #f5f5f5;
    }
    QLabel#brandLabel {
      color: #ffffff;
      font-size: 26px;
      font-weight: 700;
    }
    QLabel#avatarLabel {
      background: #2f2f2f;
      border: 2px solid #4d4d4d;
      border-radius: 36px;
      color: #c9c9c9;
      font-size: 28px;
      font-weight: 700;
    }
    QLineEdit,
    QComboBox {
      min-height: 36px;
      border: 1px solid #3d3d3d;
      border-radius: 6px;
      padding: 0 12px;
      background: #242424;
      color: #f2f2f2;
      selection-background-color: #2d7dff;
    }
    QComboBox::drop-down {
      border: 0;
      width: 28px;
    }
    QPushButton {
      min-height: 36px;
      border: 0;
      border-radius: 6px;
      background: #2d7dff;
      color: #ffffff;
      font-weight: 600;
    }
    QPushButton#registerButton {
      background: #353535;
      color: #d8d8d8;
    }
    QPushButton:disabled {
      background: #555555;
      color: #a8a8a8;
    }
  )";
}

}  // namespace

LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent) {
  setObjectName("LoginWindow");
  setWindowTitle(TextBrandName());
  setFixedSize(380, 520);
  setStyleSheet(LoginWindowStyle());

  auto *root_layout = new QVBoxLayout(this);
  auto *brand_label = new QLabel(TextBrandName(), this);
  auto *avatar_label = new QLabel(QStringLiteral("FG"), this);

  auth_client_ = new AuthClient(this);
  nickname_edit_ = new QLineEdit(this);
  username_combo_ = new QComboBox(this);
  password_edit_ = new QLineEdit(this);
  register_button_ = new QPushButton(TextRegister(), this);
  login_button_ = new QPushButton(TextLogin(), this);

  root_layout->setContentsMargins(42, 28, 42, 30);
  root_layout->setSpacing(14);

  brand_label->setObjectName("brandLabel");
  brand_label->setAlignment(Qt::AlignCenter);

  avatar_label->setObjectName("avatarLabel");
  avatar_label->setAlignment(Qt::AlignCenter);
  avatar_label->setFixedSize(72, 72);

  nickname_edit_->setPlaceholderText(TextNicknamePlaceholder());
  username_combo_->setEditable(true);
  username_combo_->setInsertPolicy(QComboBox::NoInsert);
  username_combo_->lineEdit()->setPlaceholderText(TextUsernamePlaceholder());
  password_edit_->setPlaceholderText(TextPasswordPlaceholder());
  password_edit_->setEchoMode(QLineEdit::Password);
  register_button_->setObjectName("registerButton");

  root_layout->addWidget(brand_label);
  root_layout->addSpacing(8);
  root_layout->addWidget(avatar_label, 0, Qt::AlignHCenter);
  root_layout->addSpacing(8);
  root_layout->addWidget(nickname_edit_);
  root_layout->addWidget(username_combo_);
  root_layout->addWidget(password_edit_);
  root_layout->addStretch();
  root_layout->addWidget(register_button_);
  root_layout->addWidget(login_button_);

  LoadCachedUsers();

  connect(username_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &LoginWindow::SyncNicknameFromCachedUser);
  connect(register_button_, &QPushButton::clicked, this,
          &LoginWindow::HandleRegister);
  connect(login_button_, &QPushButton::clicked, this, &LoginWindow::HandleLogin);
  connect(auth_client_, &AuthClient::loginSucceeded, this,
          [this](const QString &username, const QString &nickname) {
            const QString cached_username =
                username.isEmpty() ? CurrentUsername() : username;
            const QString cached_nickname =
                nickname.isEmpty() ? nickname_edit_->text().trimmed()
                                   : nickname;
            SaveCachedUser(cached_username, cached_nickname);
            SetAuthPending(false);
            emit loginSucceeded();
          });
  connect(auth_client_, &AuthClient::loginFailed, this,
          [this](const QString &message) {
            SetAuthPending(false);
            QMessageBox::warning(this, TextLoginFailed(), message);
          });
  connect(auth_client_, &AuthClient::registerSucceeded, this,
          [this](const QString &username, const QString &nickname) {
            const QString cached_username =
                username.isEmpty() ? CurrentUsername() : username;
            const QString cached_nickname =
                nickname.isEmpty() ? nickname_edit_->text().trimmed()
                                   : nickname;
            SaveCachedUser(cached_username, cached_nickname);
            SetAuthPending(false);
            QMessageBox::information(this, TextRegisterTitle(),
                                     TextRegisterSucceeded());
            emit loginSucceeded();
          });
  connect(auth_client_, &AuthClient::registerFailed, this,
          [this](const QString &message) {
            SetAuthPending(false);
            QMessageBox::warning(this, TextRegisterTitle(), message);
          });
}

void LoginWindow::HandleLogin() {
  const QString username = CurrentUsername();
  const QString password = password_edit_->text();

  if (username.isEmpty() || password.isEmpty()) {
    QMessageBox::warning(this, TextLoginFailed(), TextMissingLoginFields());
    return;
  }

  SetAuthPending(true);
  login_button_->setText(TextLoginPending());
  auth_client_->Login(username, password);
}

void LoginWindow::HandleRegister() {
  const QString nickname = nickname_edit_->text().trimmed();
  const QString username = CurrentUsername();
  const QString password = password_edit_->text();

  if (nickname.isEmpty() || username.isEmpty() || password.isEmpty()) {
    QMessageBox::warning(this, TextRegisterTitle(), TextMissingRegisterFields());
    return;
  }

  SetAuthPending(true);
  register_button_->setText(TextRegisterPending());
  auth_client_->Register(nickname, username, password);
}

void LoginWindow::LoadCachedUsers() {
  QSettings settings(kSettingsOrg, kSettingsApp);
  settings.beginGroup(kCachedUsersGroup);

  const QStringList usernames = settings.childKeys();
  for (const QString &username : usernames) {
    username_combo_->addItem(username, settings.value(username).toString());
  }

  settings.endGroup();

  if (username_combo_->count() > 0) {
    username_combo_->setCurrentIndex(0);
    SyncNicknameFromCachedUser(0);
  }
}

void LoginWindow::SaveCachedUser(const QString &username,
                                 const QString &nickname) {
  if (username.isEmpty()) {
    return;
  }

  QSettings settings(kSettingsOrg, kSettingsApp);
  settings.beginGroup(kCachedUsersGroup);
  settings.setValue(username, nickname);
  settings.endGroup();

  const int existing_index = username_combo_->findText(username);
  if (existing_index >= 0) {
    username_combo_->setItemData(existing_index, nickname);
    username_combo_->setCurrentIndex(existing_index);
  } else {
    username_combo_->addItem(username, nickname);
    username_combo_->setCurrentIndex(username_combo_->count() - 1);
  }
}

void LoginWindow::SyncNicknameFromCachedUser(int index) {
  if (index < 0) {
    return;
  }

  const QString nickname = username_combo_->itemData(index).toString();
  if (!nickname.isEmpty()) {
    nickname_edit_->setText(nickname);
  }
}

void LoginWindow::SetAuthPending(bool pending) {
  nickname_edit_->setEnabled(!pending);
  username_combo_->setEnabled(!pending);
  password_edit_->setEnabled(!pending);
  register_button_->setEnabled(!pending);
  login_button_->setEnabled(!pending);
  if (!pending) {
    register_button_->setText(TextRegister());
    login_button_->setText(TextLogin());
  }
}

QString LoginWindow::CurrentUsername() const {
  return username_combo_->currentText().trimmed();
}
