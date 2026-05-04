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
    name_label->setObjectName("sessionName");
    time_label->setObjectName("sessionTime");
    preview_label->setObjectName("sessionPreview");
    unread_label->setObjectName("unreadBadge");

    avatar_container->setFixedSize(46, 46);
    avatar->setFixedSize(42, 42);
    avatar->setAlignment(Qt::AlignCenter);
    online_dot->setFixedSize(8, 8);
    avatar_layout->setContentsMargins(0, 0, 0, 0);
    avatar_layout->addWidget(avatar, 0, Qt::AlignTop | Qt::AlignLeft);
    avatar_layout->addWidget(online_dot, 0, Qt::AlignRight | Qt::AlignBottom);

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
          AddConversation(target_user_id, item.value("targetUsername").toString(),
                          item.value("lastMessage").toString(),
                          item.value("lastMessageTime").toString(),
                          unread_count, item.value("isOnline").toBool());
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
    AddConversation(target_user_id, item->data(Qt::DisplayRole).toString(),
                    content, created_at, 0, item->data(kOnlineRole).toBool());
    delete session_list_->takeItem(i);
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
    session_list_->setItemWidget(existing_item, widget);
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
  item->setSizeHint({300, 70});
  session_list_->addItem(item);
  session_list_->setItemWidget(item, widget);
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
  session_list_->setItemWidget(item, widget);
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
