#include "MainWindow.h"

#include "ChatListWidget.h"
#include "ChatWindow.h"
#include "NavBar.h"
#include "TitleBar.h"

#include "../widgets/AddFriendDialog.h"
#include "../widgets/FriendListWidget.h"

#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "ApiClient.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowFlags(Qt::FramelessWindowHint);
  setAttribute(Qt::WA_TranslucentBackground, false);
  setWindowTitle(QStringLiteral("\u98de\u9e3d\u901a\u8baf"));
  resize(1180, 760);
  LoadStyleSheet();

  auto *central_widget = new QWidget(this);
  auto *root_layout = new QVBoxLayout(central_widget);
  auto *splitter = new QSplitter(Qt::Horizontal, central_widget);

  title_bar_ = new TitleBar(central_widget);
  nav_bar_ = new NavBar(splitter);
  chat_list_widget_ = new ChatListWidget(splitter);
  friend_list_widget_ = new FriendListWidget(splitter);
  chat_window_ = new ChatWindow(splitter);
  middle_stack_ = new QStackedWidget(splitter);
  request_timer_ = new QTimer(this);

  middle_stack_->addWidget(chat_list_widget_);
  middle_stack_->addWidget(friend_list_widget_);

  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);

  splitter->setObjectName("mainSplitter");
  splitter->addWidget(nav_bar_);
  splitter->addWidget(middle_stack_);
  splitter->addWidget(chat_window_);
  splitter->setSizes({60, 300, 820});
  splitter->setCollapsible(0, false);
  splitter->setCollapsible(1, false);
  splitter->setCollapsible(2, false);

  root_layout->addWidget(title_bar_);
  root_layout->addWidget(splitter, 1);

  setCentralWidget(central_widget);

  connect(nav_bar_, &NavBar::currentIndexChanged, this,
          &MainWindow::SwitchMiddlePanel);
  connect(chat_list_widget_, &ChatListWidget::conversationSelected,
          chat_window_, &ChatWindow::loadMessages);
  connect(chat_list_widget_, &ChatListWidget::newConversationRequested, this,
          &MainWindow::OpenAddFriendDialog);
  connect(friend_list_widget_, &FriendListWidget::friendActivated, this,
          &MainWindow::OpenConversation);
  connect(friend_list_widget_, &FriendListWidget::requestCountChanged, nav_bar_,
          &NavBar::setFriendBadge);
  connect(chat_window_, &ChatWindow::messageSent, chat_list_widget_,
          &ChatListWidget::updateConversationPreview);
  connect(request_timer_, &QTimer::timeout, this,
          &MainWindow::RefreshFriendRequests);

  LoadProfile();
  chat_list_widget_->loadConversations();
  friend_list_widget_->loadRequests();
  request_timer_->start(30000);
}

void MainWindow::LoadStyleSheet() {
  QFile style_file(":/MainWindow/styles/main.qss");
  if (style_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    setStyleSheet(QString::fromUtf8(style_file.readAll()));
  }
}

void MainWindow::LoadProfile() {
  ApiClient::instance()->get("/api/v1/user/profile", this,
                             [this](const QJsonObject &response) {
                               const QJsonObject data =
                                   response.value("data").toObject();
                               title_bar_->setUserInfo(
                                   data.value("username").toString(),
                                   data.value("nickname").toString(),
                                   data.value("signature").toString(),
                                   data.value("avatar").toString(),
                                   data.value("gender").toString(),
                                   data.value("birthday").toString(),
                                   data.value("createdAt").toString());
                             });
}

void MainWindow::OpenAddFriendDialog() {
  AddFriendDialog dialog(this);
  connect(&dialog, &AddFriendDialog::friendsChanged, friend_list_widget_,
          &FriendListWidget::loadFriends);
  connect(&dialog, &AddFriendDialog::friendsChanged, friend_list_widget_,
          &FriendListWidget::loadRequests);
  dialog.exec();
}

void MainWindow::OpenConversation(int target_user_id,
                                  const QString &display_name,
                                  bool is_online) {
  if (target_user_id <= 0) {
    return;
  }

  QJsonObject body;
  body.insert("targetUserId", target_user_id);
  ApiClient::instance()->post(
      "/api/v1/conversations", body, this,
      [this, target_user_id, display_name, is_online](
          const QJsonObject &response) {
        const QJsonObject data = response.value("data").toObject();
        const int user_id = data.value("targetUserId").toInt(target_user_id);
        const QString name =
            data.value("targetNickname").toString(display_name);
        const bool online = data.value("isOnline").toBool(is_online);
        SwitchMiddlePanel(0);
        nav_bar_->setCurrentIndex(0);
        chat_list_widget_->loadConversations();
        chat_window_->loadMessages(user_id, name, online);
      },
      [this]() {
        QMessageBox::warning(
            this, QString(),
            QStringLiteral("\u6253\u5f00\u4f1a\u8bdd\u5931\u8d25"));
      });
}

void MainWindow::RefreshFriendRequests() {
  friend_list_widget_->loadRequests();
}

void MainWindow::SwitchMiddlePanel(int index) {
  if (index == 1) {
    friend_list_widget_->loadFriends();
    friend_list_widget_->loadRequests();
  } else {
    chat_list_widget_->loadConversations();
  }
  middle_stack_->setCurrentIndex(index);
}
