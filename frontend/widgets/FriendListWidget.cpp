#include "FriendListWidget.h"

#include "../src/ApiClient.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace {

constexpr int kFriendUserIdRole = Qt::UserRole + 1;
constexpr int kFriendOnlineRole = Qt::UserRole + 2;

QString DisplayName(const QString &username, const QString &nickname) {
  return nickname.isEmpty() ? username : nickname;
}

class FriendItemWidget : public QWidget {
 public:
  FriendItemWidget(const QString &name, const QString &signature,
                   bool is_online, QWidget *parent = nullptr)
      : QWidget(parent) {
    auto *root_layout = new QHBoxLayout(this);
    auto *avatar = new QLabel(name.left(1).toUpper(), this);
    auto *text_layout = new QVBoxLayout();
    auto *name_label = new QLabel(name, this);
    auto *signature_label = new QLabel(signature, this);
    auto *online_dot = new QLabel(this);

    avatar->setObjectName("friendAvatar");
    name_label->setObjectName("friendName");
    signature_label->setObjectName("friendSignature");
    online_dot->setObjectName(is_online ? "onlineDot" : "offlineDot");
    avatar->setFixedSize(36, 36);
    avatar->setAlignment(Qt::AlignCenter);
    online_dot->setFixedSize(8, 8);

    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(4);
    text_layout->addWidget(name_label);
    text_layout->addWidget(signature_label);

    root_layout->setContentsMargins(12, 8, 12, 8);
    root_layout->setSpacing(10);
    root_layout->addWidget(avatar);
    root_layout->addLayout(text_layout, 1);
    root_layout->addWidget(online_dot);
  }
};

}  // namespace

FriendListWidget::FriendListWidget(QWidget *parent) : QWidget(parent) {
  setObjectName("friendListPane");
  auto *root_layout = new QVBoxLayout(this);
  auto *title = new QLabel(QStringLiteral("\u6211\u7684\u597d\u53cb"), this);

  request_panel_ = new QWidget(this);
  request_layout_ = new QVBoxLayout(request_panel_);
  friend_list_ = new QListWidget(this);

  title->setObjectName("paneTitle");
  request_panel_->setObjectName("friendRequestPanel");
  request_layout_->setContentsMargins(12, 10, 12, 10);
  request_layout_->setSpacing(8);
  request_panel_->hide();

  friend_list_->setObjectName("friendList");
  friend_list_->setFrameShape(QFrame::NoFrame);
  friend_list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);
  root_layout->addWidget(title);
  root_layout->addWidget(request_panel_);
  root_layout->addWidget(friend_list_, 1);

  connect(friend_list_, &QListWidget::itemDoubleClicked, this,
          [this](QListWidgetItem *item) {
            emit friendActivated(item->data(kFriendUserIdRole).toInt(),
                                 item->data(Qt::DisplayRole).toString(),
                                 item->data(kFriendOnlineRole).toBool());
          });
  connect(friend_list_, &QListWidget::itemClicked, this,
          [this](QListWidgetItem *item) {
            emit friendActivated(item->data(kFriendUserIdRole).toInt(),
                                 item->data(Qt::DisplayRole).toString(),
                                 item->data(kFriendOnlineRole).toBool());
          });
}

void FriendListWidget::loadFriends() {
  friend_list_->clear();
  ApiClient::instance()->get("/api/v1/friends", this,
                             [this](const QJsonObject &response) {
                               const QJsonArray friends =
                                   response.value("data").toArray();
                               for (const QJsonValue &value : friends) {
                                 const QJsonObject item = value.toObject();
                                 AddFriend(item.value("userId").toInt(),
                                           item.value("username").toString(),
                                           item.value("nickname").toString(),
                                           item.value("signature").toString(),
                                           item.value("isOnline").toBool());
                               }
                             });
}

void FriendListWidget::loadRequests() {
  ApiClient::instance()->get("/api/v1/friends/requests", this,
                             [this](const QJsonObject &response) {
                               RenderRequests(response.value("data").toArray());
                             },
                             [this]() {
                               RenderRequests(QJsonArray());
                             });
}

void FriendListWidget::AddFriend(int user_id, const QString &username,
                                 const QString &nickname,
                                 const QString &signature, bool is_online) {
  const QString name = DisplayName(username, nickname);
  auto *item = new QListWidgetItem();
  auto *widget = new FriendItemWidget(name, signature, is_online, friend_list_);
  item->setData(kFriendUserIdRole, user_id);
  item->setData(Qt::DisplayRole, name);
  item->setData(kFriendOnlineRole, is_online);
  item->setSizeHint({300, 64});
  friend_list_->addItem(item);
  friend_list_->setItemWidget(item, widget);
}

void FriendListWidget::RenderRequests(const QJsonArray &requests) {
  while (request_layout_->count() > 0) {
    QLayoutItem *item = request_layout_->takeAt(0);
    if (item->widget() != nullptr) {
      item->widget()->deleteLater();
    }
    delete item;
  }

  request_panel_->setVisible(!requests.isEmpty());
  emit requestCountChanged(requests.size());
  if (requests.isEmpty()) {
    return;
  }

  auto *title = new QLabel(QStringLiteral("\u597d\u53cb\u7533\u8bf7"),
                           request_panel_);
  title->setObjectName("requestTitle");
  request_layout_->addWidget(title);

  for (const QJsonValue &value : requests) {
    const QJsonObject request = value.toObject();
    const int request_id = request.value("requestId").toInt();
    const QString nickname =
        DisplayName(request.value("fromUsername").toString(),
                    request.value("fromNickname").toString());
    auto *row = new QWidget(request_panel_);
    auto *row_layout = new QHBoxLayout(row);
    auto *name_label = new QLabel(nickname, row);
    auto *accept_button = new QPushButton(QStringLiteral("\u540c\u610f"), row);
    auto *reject_button = new QPushButton(QStringLiteral("\u62d2\u7edd"), row);

    accept_button->setObjectName("acceptButton");
    reject_button->setObjectName("secondaryButton");
    row_layout->setContentsMargins(0, 0, 0, 0);
    row_layout->addWidget(name_label, 1);
    row_layout->addWidget(accept_button);
    row_layout->addWidget(reject_button);
    request_layout_->addWidget(row);

    connect(accept_button, &QPushButton::clicked, this,
            [this, request_id]() { HandleRequest(request_id, 1); });
    connect(reject_button, &QPushButton::clicked, this,
            [this, request_id]() { HandleRequest(request_id, 2); });
  }
}

void FriendListWidget::HandleRequest(int request_id, int action) {
  QJsonObject body;
  body.insert("requestId", request_id);
  body.insert("action", action);
  ApiClient::instance()->post("/api/v1/friends/handle", body, this,
                              [this](const QJsonObject &) {
                                loadRequests();
                                loadFriends();
                              });
}
