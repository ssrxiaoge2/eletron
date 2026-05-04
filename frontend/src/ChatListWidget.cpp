#include "ChatListWidget.h"

#include "ApiClient.h"

#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

namespace {

constexpr int kTargetUserIdRole = Qt::UserRole + 1;
constexpr int kOnlineRole = Qt::UserRole + 2;
constexpr int kLastMessageRole = Qt::UserRole + 3;
constexpr int kLastMessageTimeRole = Qt::UserRole + 4;
constexpr int kUnreadCountRole = Qt::UserRole + 5;

bool IsOnline(const QJsonObject &item) {
  return item.value("isOnline").toBool(false);
}

QString OnlineDotStyle(bool is_online) {
  return QStringLiteral("border-radius: 4px; background-color: %1;")
      .arg(is_online ? QStringLiteral("#26c35a") : QStringLiteral("#9a9a9a"));
}

class SessionItemWidget : public QWidget {
 public:
  SessionItemWidget(const QString &name, const QString &time,
                    const QString &preview, int unread_count, bool is_online,
                    QWidget *parent = nullptr)
      : QWidget(parent) {
    auto *root_layout = new QHBoxLayout(this);
    auto *avatar_container = new QWidget(this);
    auto *avatar_layout = new QVBoxLayout(avatar_container);
    auto *avatar = new QLabel(name.left(1).toUpper(), avatar_container);
    auto *online_dot = new QLabel(avatar_container);
    auto *text_layout = new QVBoxLayout();
    auto *top_layout = new QHBoxLayout();
    auto *name_label = new QLabel(name, this);
    auto *time_label = new QLabel(time, this);
    auto *bottom_layout = new QHBoxLayout();
    auto *preview_label = new QLabel(preview, this);
    auto *unread_label = new QLabel(QString::number(unread_count), this);

    setObjectName("sessionItem");
    avatar->setObjectName("sessionAvatar");
    online_dot->setObjectName(is_online ? "sessionOnlineDot"
                                        : "sessionOfflineDot");
    online_dot->setStyleSheet(OnlineDotStyle(is_online));
    online_dot->setAttribute(Qt::WA_StyledBackground, true);
    name_label->setObjectName("sessionName");
    time_label->setObjectName("sessionTime");
    preview_label->setObjectName("sessionPreview");
    unread_label->setObjectName("unreadBadge");

    avatar_container->setFixedSize(46, 46);
    avatar->setFixedSize(42, 42);
    avatar->setAlignment(Qt::AlignCenter);
    online_dot->setFixedSize(8, 8);
    avatar_layout->setContentsMargins(0, 0, 0, 0);
    avatar_layout->setSpacing(0);
    avatar_layout->addWidget(avatar, 0, Qt::AlignTop | Qt::AlignLeft);
    online_dot->raise();
    online_dot->move(34, 34);

    preview_label->setTextFormat(Qt::PlainText);
    preview_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    unread_label->setVisible(unread_count > 0);
    unread_label->setAlignment(Qt::AlignCenter);

    top_layout->setContentsMargins(0, 0, 0, 0);
    top_layout->addWidget(name_label);
    top_layout->addStretch();
    top_layout->addWidget(time_label);

    bottom_layout->setContentsMargins(0, 0, 0, 0);
    bottom_layout->addWidget(preview_label);
    bottom_layout->addStretch();
    bottom_layout->addWidget(unread_label);

    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(4);
    text_layout->addLayout(top_layout);
    text_layout->addLayout(bottom_layout);

    root_layout->setContentsMargins(12, 8, 10, 8);
    root_layout->setSpacing(8);
    root_layout->addWidget(avatar_container);
    root_layout->addLayout(text_layout, 1);
  }
};

}  // namespace

ChatListWidget::ChatListWidget(QWidget *parent) : QWidget(parent) {
  setObjectName("chatListPane");
  setMinimumWidth(240);

  auto *root_layout = new QVBoxLayout(this);
  auto *search_bar = new QWidget(this);
  auto *search_layout = new QHBoxLayout(search_bar);
  auto *search_edit = new QLineEdit(search_bar);
  auto *new_chat_button = new QToolButton(search_bar);

  session_list_ = new QListWidget(this);

  search_bar->setObjectName("searchBar");
  search_edit->setObjectName("searchEdit");
  search_edit->setPlaceholderText(QStringLiteral("\u641c\u7d22"));
  new_chat_button->setObjectName("newChatButton");
  new_chat_button->setText("+");
  new_chat_button->setFixedSize(32, 32);

  search_layout->setContentsMargins(12, 8, 12, 8);
  search_layout->setSpacing(8);
  search_layout->addWidget(search_edit);
  search_layout->addWidget(new_chat_button);

  session_list_->setObjectName("sessionList");
  session_list_->setFrameShape(QFrame::NoFrame);
  session_list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);
  root_layout->addWidget(search_bar);
  root_layout->addWidget(session_list_, 1);

  connect(session_list_, &QListWidget::itemClicked, this,
          [this](QListWidgetItem *item) {
            ClearUnreadBadge(item);
            emit conversationSelected(
                item->data(kTargetUserIdRole).toInt(),
                item->data(Qt::DisplayRole).toString(),
                item->data(kOnlineRole).toBool());
          });
  connect(new_chat_button, &QToolButton::clicked, this,
          &ChatListWidget::newConversationRequested);
}

void ChatListWidget::loadConversations() {
  session_list_->clear();
  friend_nickname_cache_.clear();
  friend_online_cache_.clear();
  refreshConversations();
}

void ChatListWidget::refreshConversations() {
  ApiClient::instance()->get(
      "/api/v1/friends", this,
      [this](const QJsonObject &response) {
        const QJsonArray friends = response.value("data").toArray();
        for (const QJsonValue &value : friends) {
          const QJsonObject item = value.toObject();
          const int user_id = item.value("userId").toInt();
          const QString nickname = item.value("nickname").toString();
          if (user_id > 0 && !nickname.isEmpty()) {
            friend_nickname_cache_.insert(user_id, nickname);
          }
          if (user_id > 0) {
            friend_online_cache_.insert(user_id, IsOnline(item));
          }
        }
        FetchConversations();
      },
      [this]() { FetchConversations(); });
}

void ChatListWidget::refreshOnlineStatuses() {
  ApiClient::instance()->get("/api/v1/friends", this,
                             [this](const QJsonObject &response) {
                               const QJsonArray friends =
                                   response.value("data").toArray();
                               for (const QJsonValue &value : friends) {
                                 const QJsonObject item = value.toObject();
                                 const int user_id =
                                     item.value("userId").toInt();
                                 if (user_id <= 0) {
                                   continue;
                                 }
                                 const QString nickname =
                                     item.value("nickname").toString();
                                 if (!nickname.isEmpty()) {
                                   friend_nickname_cache_.insert(user_id,
                                                                 nickname);
                                 }
                                 const bool is_online = IsOnline(item);
                                 friend_online_cache_.insert(user_id,
                                                             is_online);
                                 ApplyOnlineStatus(user_id, is_online);
                               }
                             });
}

void ChatListWidget::FetchConversations() {
  ApiClient::instance()->get(
      "/api/v1/conversations", this,
      [this](const QJsonObject &response) {
        const QJsonArray conversations =
            response.value("data").toArray();
        QSet<int> rendered_user_ids;
        for (const QJsonValue &value : conversations) {
          const QJsonObject item = value.toObject();
          const int target_user_id = item.value("targetUserId").toInt();
          if (target_user_id <= 0 || rendered_user_ids.contains(target_user_id)) {
            continue;
          }
          rendered_user_ids.insert(target_user_id);
          const int unread_count =
              read_conversation_ids_.contains(target_user_id)
                  ? 0
                  : item.value("unreadCount").toInt();
          AddConversation(target_user_id, DisplayNameForConversation(item),
                          item.value("lastMessage").toString(),
                          item.value("lastMessageTime").toString(),
                          unread_count,
                          OnlineStateForConversation(target_user_id, item));
        }
      });
}

bool ChatListWidget::activateConversation(int target_user_id) {
  for (int i = 0; i < session_list_->count(); ++i) {
    QListWidgetItem *item = session_list_->item(i);
    if (item->data(kTargetUserIdRole).toInt() != target_user_id) {
      continue;
    }
    session_list_->setCurrentItem(item);
    ClearUnreadBadge(item);
    emit conversationSelected(target_user_id, item->data(Qt::DisplayRole).toString(),
                              item->data(kOnlineRole).toBool());
    return true;
  }
  return false;
}

bool ChatListWidget::hasConversation(int target_user_id) const {
  for (int i = 0; i < session_list_->count(); ++i) {
    if (session_list_->item(i)->data(kTargetUserIdRole).toInt() ==
        target_user_id) {
      return true;
    }
  }
  return false;
}

void ChatListWidget::updateConversationPreview(int target_user_id,
                                               const QString &content,
                                               const QString &created_at) {
  for (int i = 0; i < session_list_->count(); ++i) {
    QListWidgetItem *item = session_list_->item(i);
    if (item->data(kTargetUserIdRole).toInt() != target_user_id) {
      continue;
    }
    const QString display_name = item->data(Qt::DisplayRole).toString();
    const bool is_online = item->data(kOnlineRole).toBool();
    QWidget *old_widget = session_list_->itemWidget(item);
    session_list_->removeItemWidget(item);
    if (old_widget != nullptr) {
      old_widget->deleteLater();
    }
    QListWidgetItem *moved_item = session_list_->takeItem(i);
    moved_item->setData(Qt::DisplayRole, display_name);
    moved_item->setData(kOnlineRole, is_online);
    moved_item->setData(kLastMessageRole, content);
    moved_item->setData(kLastMessageTimeRole, created_at);
    moved_item->setData(kUnreadCountRole, 0);
    auto *widget = new SessionItemWidget(display_name, FormatTime(created_at),
                                         content, 0, is_online, session_list_);
    session_list_->insertItem(0, moved_item);
    session_list_->setItemWidget(moved_item, widget);
    session_list_->setCurrentItem(moved_item);
    return;
  }
}

void ChatListWidget::AddConversation(int target_user_id,
                                     const QString &target_username,
                                     const QString &last_message,
                                     const QString &last_message_time,
                                     int unread_count, bool is_online) {
  for (int i = 0; i < session_list_->count(); ++i) {
    QListWidgetItem *existing_item = session_list_->item(i);
    if (existing_item->data(kTargetUserIdRole).toInt() != target_user_id) {
      continue;
    }
    const bool online_changed =
        existing_item->data(kOnlineRole).toBool() != is_online;
    QWidget *old_widget = session_list_->itemWidget(existing_item);
    session_list_->removeItemWidget(existing_item);
    if (old_widget != nullptr) {
      old_widget->deleteLater();
    }
    auto *widget = new SessionItemWidget(
        target_username, FormatTime(last_message_time), last_message,
        unread_count, is_online, session_list_);
    existing_item->setData(Qt::DisplayRole, target_username);
    existing_item->setData(kOnlineRole, is_online);
    existing_item->setData(kLastMessageRole, last_message);
    existing_item->setData(kLastMessageTimeRole, last_message_time);
    existing_item->setData(kUnreadCountRole, unread_count);
    session_list_->setItemWidget(existing_item, widget);
    if (online_changed) {
      emit onlineStatusChanged(target_user_id, is_online);
    }
    return;
  }

  auto *item = new QListWidgetItem();
  auto *widget = new SessionItemWidget(
      target_username, FormatTime(last_message_time), last_message,
      unread_count, is_online, session_list_);
  item->setData(kTargetUserIdRole, target_user_id);
  item->setData(Qt::DisplayRole, target_username);
  item->setData(kOnlineRole, is_online);
  item->setData(kLastMessageRole, last_message);
  item->setData(kLastMessageTimeRole, last_message_time);
  item->setData(kUnreadCountRole, unread_count);
  item->setSizeHint({300, 70});
  session_list_->addItem(item);
  session_list_->setItemWidget(item, widget);
}

QString ChatListWidget::DisplayNameForConversation(
    const QJsonObject &item) const {
  const int target_user_id = item.value("targetUserId").toInt();
  const QString cached_nickname = friend_nickname_cache_.value(target_user_id);
  if (!cached_nickname.isEmpty()) {
    return cached_nickname;
  }

  const QString nickname = item.value("targetNickname").toString(
      item.value("nickname").toString());
  if (!nickname.isEmpty()) {
    return nickname;
  }
  return item.value("targetUsername").toString();
}

void ChatListWidget::ClearUnreadBadge(QListWidgetItem *item) {
  const int target_user_id = item->data(kTargetUserIdRole).toInt();
  read_conversation_ids_.insert(target_user_id);
  MarkConversationRead(target_user_id);
  const QString target_username = item->data(Qt::DisplayRole).toString();
  const QString last_message = item->data(kLastMessageRole).toString();
  const QString last_message_time = item->data(kLastMessageTimeRole).toString();
  const bool is_online = item->data(kOnlineRole).toBool();
  QWidget *old_widget = session_list_->itemWidget(item);
  session_list_->removeItemWidget(item);
  if (old_widget != nullptr) {
    old_widget->deleteLater();
  }
  auto *widget = new SessionItemWidget(target_username, FormatTime(last_message_time),
                                       last_message, 0, is_online, session_list_);
  item->setData(kTargetUserIdRole, target_user_id);
  item->setData(Qt::DisplayRole, target_username);
  item->setData(kOnlineRole, is_online);
  item->setData(kLastMessageRole, last_message);
  item->setData(kLastMessageTimeRole, last_message_time);
  item->setData(kUnreadCountRole, 0);
  session_list_->setItemWidget(item, widget);
}

void ChatListWidget::clearUnreadForConversation(int target_user_id) {
  for (int i = 0; i < session_list_->count(); ++i) {
    QListWidgetItem *item = session_list_->item(i);
    if (item->data(kTargetUserIdRole).toInt() == target_user_id) {
      ClearUnreadBadge(item);
      return;
    }
  }
  read_conversation_ids_.insert(target_user_id);
  MarkConversationRead(target_user_id);
}

void ChatListWidget::MarkConversationRead(int target_user_id) {
  if (target_user_id <= 0 || read_request_pending_ids_.contains(target_user_id)) {
    return;
  }

  read_request_pending_ids_.insert(target_user_id);
  QJsonObject body;
  body.insert("targetUserId", target_user_id);
  ApiClient::instance()->post(
      "/api/v1/conversations/read", body, this,
      [this, target_user_id](const QJsonObject &) {
        read_request_pending_ids_.remove(target_user_id);
      },
      [this, target_user_id, body]() {
        ApiClient::instance()->post(
            "/api/v1/messages/read", body, this,
            [this, target_user_id](const QJsonObject &) {
              read_request_pending_ids_.remove(target_user_id);
            },
            [this, target_user_id]() {
              read_request_pending_ids_.remove(target_user_id);
            });
      });
}

QString ChatListWidget::FormatTime(const QString &raw_time) const {
  const QDateTime date_time =
      QDateTime::fromString(raw_time, "yyyy-MM-dd HH:mm:ss");
  if (date_time.isValid()) {
    return date_time.time().toString("HH:mm");
  }
  return raw_time.left(5);
}

bool ChatListWidget::OnlineStateForConversation(
    int target_user_id, const QJsonObject &item) const {
  const bool conversation_online = IsOnline(item);
  if (friend_online_cache_.contains(target_user_id)) {
    return friend_online_cache_.value(target_user_id) || conversation_online;
  }
  return conversation_online;
}

void ChatListWidget::ApplyOnlineStatus(int target_user_id, bool is_online) {
  for (int i = 0; i < session_list_->count(); ++i) {
    QListWidgetItem *item = session_list_->item(i);
    if (item->data(kTargetUserIdRole).toInt() != target_user_id) {
      continue;
    }

    AddConversation(target_user_id, item->data(Qt::DisplayRole).toString(),
                    item->data(kLastMessageRole).toString(),
                    item->data(kLastMessageTimeRole).toString(),
                    item->data(kUnreadCountRole).toInt(), is_online);
    return;
  }
}
