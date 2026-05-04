#include "LoginWindow.h"

#include "AuthClient.h"

#include <QtCore/QSettings>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>

namespace {

constexpr char kSettingsOrg[] = "Feige";
constexpr char kSettingsApp[] = "DesktopIM";
constexpr char kCachedUsersGroup[] = "login_users";
constexpr char kNicknameKey[] = "nickname";
constexpr char kTokenKey[] = "token";
constexpr char kLegacyPasswordKey[] = "password";
constexpr int kCachedModeIndex = 0;
constexpr int kPasswordModeIndex = 1;

QString TextBrandName() {
  return QStringLiteral("\u98de\u9e3d\u901a\u8baf");
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

QString TextPasswordLogin() {
  return QStringLiteral("\u8d26\u53f7\u5bc6\u7801\u767b\u5f55");
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

QString TextNoCachedToken() {
  return QStringLiteral(
      "\u8be5\u8d26\u53f7\u6ca1\u6709\u53ef\u7528\u7684 token"
      "\uff0c\u8bf7\u4f7f\u7528\u8d26\u53f7\u5bc6\u7801\u767b\u5f55");
}

QString TextUsernamePlaceholder() {
  return QStringLiteral("\u8f93\u5165\u7528\u6237\u540d");
}

QString TextPasswordPlaceholder() {
  return QStringLiteral("\u8f93\u5165\u5bc6\u7801");
}

QString DisplayName(const QString &username, const QString &nickname) {
  const QString visible_name = nickname.isEmpty() ? username : nickname;
  if (visible_name == username) {
    return username;
  }
  return QStringLiteral("%1  %2").arg(visible_name, username);
}

QString LoginWindowStyle() {
  return R"(
    LoginWindow {
      background: #241433;
      color: #f5f5f5;
    }
    QLabel#brandLabel {
      color: #f8f6ff;
      font-size: 28px;
      font-weight: 700;
    }
    QLabel#avatarLabel {
      background: #2e2e2e;
      border: 2px solid #ffffff;
      border-radius: 43px;
      color: #cfcfcf;
      font-size: 28px;
      font-weight: 700;
    }
    QLineEdit,
    QComboBox {
      min-height: 38px;
      border: 1px solid #4a4056;
      border-radius: 7px;
      padding: 0 12px;
      background: #30253b;
      color: #f2f2f2;
      selection-background-color: #2d7dff;
    }
    QComboBox {
      min-height: 42px;
      font-size: 16px;
    }
    QComboBox::drop-down {
      border: 0;
      width: 30px;
    }
    QPushButton {
      min-height: 40px;
      border: 0;
      border-radius: 8px;
      background: #087dff;
      color: #ffffff;
      font-weight: 600;
    }
    QPushButton#secondaryButton,
    QPushButton#registerButton {
      background: transparent;
      color: #43a1ff;
      font-weight: 500;
    }
    QPushButton#registerButton {
      border-left: 1px solid #3b5575;
      border-radius: 0;
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
  setFixedSize(400, 560);
  setStyleSheet(LoginWindowStyle());

  auto *root_layout = new QVBoxLayout(this);
  auto *brand_label = new QLabel(TextBrandName(), this);
  mode_stack_ = new QStackedWidget(this);
  avatar_label_ = new QLabel(this);

  auto *cached_page = new QWidget(this);
  auto *cached_layout = new QVBoxLayout(cached_page);
  auto *password_page = new QWidget(this);
  auto *password_layout = new QVBoxLayout(password_page);
  auto *link_layout = new QHBoxLayout();

  auth_client_ = new AuthClient(this);
  cached_account_combo_ = new QComboBox(this);
  username_combo_ = new QComboBox(this);
  password_edit_ = new QLineEdit(this);
  cached_login_button_ = new QPushButton(TextLogin(), this);
  password_login_button_ = new QPushButton(TextPasswordLogin(), this);
  cached_register_button_ = new QPushButton(TextRegister(), this);
  register_button_ = new QPushButton(TextRegister(), this);
  login_button_ = new QPushButton(TextLogin(), this);

  root_layout->setContentsMargins(40, 26, 40, 26);
  root_layout->setSpacing(18);

  brand_label->setObjectName("brandLabel");
  brand_label->setAlignment(Qt::AlignCenter);

  avatar_label_->setObjectName("avatarLabel");
  avatar_label_->setAlignment(Qt::AlignCenter);
  avatar_label_->setFixedSize(86, 86);
  avatar_label_->setText(QString());

  cached_account_combo_->setEditable(false);
  username_combo_->setEditable(true);
  username_combo_->setInsertPolicy(QComboBox::NoInsert);
  username_combo_->lineEdit()->setPlaceholderText(TextUsernamePlaceholder());
  password_edit_->setPlaceholderText(TextPasswordPlaceholder());
  password_edit_->setEchoMode(QLineEdit::Password);
  password_login_button_->setObjectName("secondaryButton");
  cached_register_button_->setObjectName("registerButton");
  register_button_->setObjectName("registerButton");

  cached_layout->setContentsMargins(0, 0, 0, 0);
  cached_layout->setSpacing(18);
  cached_layout->addWidget(cached_account_combo_);
  cached_layout->addWidget(cached_login_button_);
  cached_layout->addStretch();

  link_layout->setContentsMargins(36, 0, 36, 0);
  link_layout->setSpacing(0);
  link_layout->addWidget(password_login_button_);
  link_layout->addWidget(cached_register_button_);
  cached_layout->addLayout(link_layout);

  password_layout->setContentsMargins(0, 0, 0, 0);
  password_layout->setSpacing(14);
  password_layout->addWidget(username_combo_);
  password_layout->addWidget(password_edit_);
  password_layout->addStretch();
  password_layout->addWidget(register_button_);
  password_layout->addWidget(login_button_);

  mode_stack_->addWidget(cached_page);
  mode_stack_->addWidget(password_page);

  root_layout->addWidget(brand_label);
  root_layout->addSpacing(8);
  root_layout->addWidget(avatar_label_, 0, Qt::AlignHCenter);
  root_layout->addSpacing(10);
  root_layout->addWidget(mode_stack_);

  LoadCachedUsers();
  if (HasCachedTokens()) {
    ShowCachedLogin();
  } else {
    ShowPasswordLogin();
  }

  connect(cached_account_combo_,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &LoginWindow::SyncCachedUser);
  connect(username_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &LoginWindow::SyncPasswordUser);
  connect(cached_login_button_, &QPushButton::clicked, this,
          &LoginWindow::HandleCachedLogin);
  connect(password_login_button_, &QPushButton::clicked, this,
          &LoginWindow::ShowPasswordLogin);
  connect(cached_register_button_, &QPushButton::clicked, this,
          &LoginWindow::HandleRegister);
  connect(register_button_, &QPushButton::clicked, this,
          &LoginWindow::HandleRegister);
  connect(login_button_, &QPushButton::clicked, this, &LoginWindow::HandleLogin);
  connect(auth_client_, &AuthClient::loginSucceeded, this,
          [this](const QString &username, const QString &nickname,
                 const QString &token) {
            quick_login_username_.clear();
            const QString cached_username =
                username.isEmpty() ? CurrentUsername() : username;
            SaveCachedUser(cached_username, nickname, token);
            SetAuthPending(false);
            emit loginSucceeded();
          });
  connect(auth_client_, &AuthClient::quickLoginTokenExpired, this, [this]() {
    RemoveCachedToken(quick_login_username_);
    username_combo_->setCurrentText(quick_login_username_);
    password_edit_->clear();
    ShowPasswordLogin();
  });
  connect(auth_client_, &AuthClient::loginFailed, this,
          [this](const QString &message) {
            quick_login_username_.clear();
            SetAuthPending(false);
            QMessageBox::warning(this, TextLoginFailed(), message);
          });
  connect(auth_client_, &AuthClient::registerSucceeded, this,
          [this](const QString &username, const QString &nickname,
                 const QString &token) {
            const QString cached_username =
                username.isEmpty() ? CurrentUsername() : username;
            SaveCachedUser(cached_username, nickname, token);
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

void LoginWindow::ResetAfterLogout(const QString &username) {
  if (!username.isEmpty()) {
    RemoveCachedToken(username);
    username_combo_->setCurrentText(username);
  }
  password_edit_->clear();
  SetAuthPending(false);
  ShowPasswordLogin();
}

void LoginWindow::HandleCachedLogin() {
  const QString username = CurrentCachedUsername();
  const QString token = CachedToken(username);

  if (username.isEmpty() || token.isEmpty()) {
    QMessageBox::warning(this, TextLoginFailed(), TextNoCachedToken());
    ShowPasswordLogin();
    return;
  }

  quick_login_username_ = username;
  username_combo_->setCurrentText(username);
  password_edit_->clear();
  SetAuthPending(true);
  cached_login_button_->setText(TextLoginPending());
  auth_client_->QuickLogin(token);
}

void LoginWindow::ShowPasswordLogin() {
  mode_stack_->setCurrentIndex(kPasswordModeIndex);
  avatar_label_->setText(QString());
}

void LoginWindow::ShowCachedLogin() {
  mode_stack_->setCurrentIndex(kCachedModeIndex);
  SyncCachedUser(cached_account_combo_->currentIndex());
}

void LoginWindow::HandleRegister() {
  if (mode_stack_->currentIndex() == kCachedModeIndex) {
    username_combo_->setCurrentText(QString());
    password_edit_->clear();
    ShowPasswordLogin();
    username_combo_->setFocus();
    return;
  }

  const QString username = CurrentUsername();
  const QString password = password_edit_->text();

  if (username.isEmpty() || password.isEmpty()) {
    QMessageBox::warning(this, TextRegisterTitle(), TextMissingLoginFields());
    return;
  }

  SetAuthPending(true);
  register_button_->setText(TextRegisterPending());
  auth_client_->Register(QString(), username, password);
}

void LoginWindow::LoadCachedUsers() {
  QSettings settings(kSettingsOrg, kSettingsApp);
  settings.beginGroup(kCachedUsersGroup);

  const QStringList usernames = settings.childGroups();
  for (const QString &username : usernames) {
    settings.beginGroup(username);
    const QString nickname = settings.value(kNicknameKey).toString();
    const QString token = settings.value(kTokenKey).toString();
    settings.remove(kLegacyPasswordKey);
    if (!token.isEmpty()) {
      cached_account_combo_->addItem(DisplayName(username, nickname), username);
    }
    username_combo_->addItem(username, username);
    settings.endGroup();
  }

  settings.endGroup();

  if (cached_account_combo_->count() > 0) {
    cached_account_combo_->setCurrentIndex(0);
    SyncCachedUser(0);
  }
}

void LoginWindow::SaveCachedUser(const QString &username,
                                 const QString &nickname,
                                 const QString &token) {
  if (username.isEmpty()) {
    return;
  }

  const QString visible_nickname = nickname.isEmpty() ? username : nickname;
  QSettings settings(kSettingsOrg, kSettingsApp);
  settings.beginGroup(kCachedUsersGroup);
  settings.beginGroup(username);
  settings.setValue(kNicknameKey, visible_nickname);
  settings.setValue(kTokenKey, token);
  settings.remove(kLegacyPasswordKey);
  settings.endGroup();
  settings.endGroup();

  const int existing_index = cached_account_combo_->findData(username);
  const QString display_name = DisplayName(username, visible_nickname);
  if (existing_index >= 0) {
    cached_account_combo_->setItemText(existing_index, display_name);
    cached_account_combo_->setCurrentIndex(existing_index);
  } else {
    cached_account_combo_->addItem(display_name, username);
    cached_account_combo_->setCurrentIndex(cached_account_combo_->count() - 1);
  }

  if (username_combo_->findData(username) < 0) {
    username_combo_->addItem(username, username);
  }
}

void LoginWindow::RemoveCachedToken(const QString &username) {
  if (username.isEmpty()) {
    return;
  }

  QSettings settings(kSettingsOrg, kSettingsApp);
  settings.beginGroup(kCachedUsersGroup);
  settings.beginGroup(username);
  settings.remove(kTokenKey);
  settings.endGroup();
  settings.endGroup();

  const int cached_index = cached_account_combo_->findData(username);
  if (cached_index >= 0) {
    cached_account_combo_->removeItem(cached_index);
  }
}

void LoginWindow::SyncCachedUser(int index) {
  if (index < 0) {
    avatar_label_->setText(QString());
    return;
  }

  const QString username = cached_account_combo_->itemData(index).toString();
  QSettings settings(kSettingsOrg, kSettingsApp);
  settings.beginGroup(kCachedUsersGroup);
  settings.beginGroup(username);
  const QString nickname = settings.value(kNicknameKey).toString();
  settings.endGroup();
  settings.endGroup();

  const QString avatar_text = (nickname.isEmpty() ? username : nickname).left(1);
  avatar_label_->setText(avatar_text.toUpper());
}

void LoginWindow::SyncPasswordUser(int index) {
  if (index < 0) {
    avatar_label_->setText(QString());
    return;
  }

  const QString username = username_combo_->itemData(index).toString();
  if (username.isEmpty()) {
    avatar_label_->setText(QString());
    return;
  }

  QSettings settings(kSettingsOrg, kSettingsApp);
  settings.beginGroup(kCachedUsersGroup);
  settings.beginGroup(username);
  const QString nickname = settings.value(kNicknameKey).toString();
  settings.endGroup();
  settings.endGroup();

  const QString avatar_text = (nickname.isEmpty() ? username : nickname).left(1);
  avatar_label_->setText(avatar_text.toUpper());
  password_edit_->clear();
}

void LoginWindow::SetAuthPending(bool pending) {
  cached_account_combo_->setEnabled(!pending);
  cached_login_button_->setEnabled(!pending);
  password_login_button_->setEnabled(!pending);
  cached_register_button_->setEnabled(!pending);
  username_combo_->setEnabled(!pending);
  password_edit_->setEnabled(!pending);
  register_button_->setEnabled(!pending);
  login_button_->setEnabled(!pending);
  if (!pending) {
    cached_login_button_->setText(TextLogin());
    cached_register_button_->setText(TextRegister());
    register_button_->setText(TextRegister());
    login_button_->setText(TextLogin());
  }
}

QString LoginWindow::CurrentUsername() const {
  return username_combo_->currentText().trimmed();
}

QString LoginWindow::CurrentCachedUsername() const {
  return cached_account_combo_->currentData().toString();
}

QString LoginWindow::CachedToken(const QString &username) const {
  QSettings settings(kSettingsOrg, kSettingsApp);
  settings.beginGroup(kCachedUsersGroup);
  settings.beginGroup(username);
  const QString token = settings.value(kTokenKey).toString();
  settings.endGroup();
  settings.endGroup();
  return token;
}

bool LoginWindow::HasCachedTokens() const {
  return cached_account_combo_->count() > 0;
}
