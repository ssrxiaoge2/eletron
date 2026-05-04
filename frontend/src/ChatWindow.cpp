#include "ChatWindow.h"

#include "ApiClient.h"
#include "MessageBubble.h"

#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QStyle>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

namespace {

QToolButton *CreateToolButton(QWidget *parent, const QString &text,
                              const QIcon &icon = QIcon()) {
  auto *button = new QToolButton(parent);
  button->setObjectName("chatToolButton");
  button->setText(text);
  button->setIcon(icon);
  button->setIconSize({20, 20});
  return button;
}

QString SendFailedText() {
  return QStringLiteral("\u53d1\u9001\u5931\u8d25\uff0c\u8bf7\u91cd\u8bd5");
}

QString OnlineDotStyle(bool is_online) {
  return QStringLiteral("border-radius: 4px; background-color: %1;")
      .arg(is_online ? QStringLiteral("#26c35a") : QStringLiteral("#9a9a9a"));
}

}  // namespace

ChatWindow::ChatWindow(QWidget *parent) : QWidget(parent) {
  setObjectName("chatWindow");

  auto *root_layout = new QVBoxLayout(this);
  message_refresh_timer_ = new QTimer(this);
  message_refresh_timer_->setInterval(2000);

  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);
  root_layout->addWidget(CreateTitleBar());
  root_layout->addWidget(CreateMessageArea(), 1);
  root_layout->addWidget(CreateInputArea());

  connect(message_refresh_timer_, &QTimer::timeout, this,
          &ChatWindow::RefreshCurrentMessages);
}

void ChatWindow::loadMessages(int target_user_id, const QString &target_username,
                              bool is_online) {
  current_target_user_id_ = target_user_id;
  current_target_username_ = target_username;
  name_label_->setText(target_username);
  updateOnlineStatus(target_user_id, is_online);

  message_request_pending_ = false;
  rendered_message_keys_.clear();
  last_rendered_minute_.clear();
  RemoveAllBubbles();
  FetchMessages(true);
  message_refresh_timer_->start();
}

void ChatWindow::updateOnlineStatus(int target_user_id, bool is_online) {
  if (target_user_id != current_target_user_id_) {
    return;
  }
  online_dot_->setObjectName(is_online ? "onlineDot" : "offlineDot");
  online_dot_->setStyleSheet(OnlineDotStyle(is_online));
  online_dot_->setAttribute(Qt::WA_StyledBackground, true);
  online_dot_->style()->unpolish(online_dot_);
  online_dot_->style()->polish(online_dot_);
}

QWidget *ChatWindow::CreateTitleBar() {
  auto *title_bar = new QWidget(this);
  auto *layout = new QHBoxLayout(title_bar);

  title_bar->setObjectName("chatTitleBar");
  name_label_ = new QLabel(title_bar);
  online_dot_ = new QLabel(title_bar);
  name_label_->setObjectName("chatTitleName");
  online_dot_->setObjectName("offlineDot");
  online_dot_->setFixedSize(8, 8);

  layout->setContentsMargins(20, 0, 16, 0);
  layout->setSpacing(8);
  layout->addWidget(name_label_);
  layout->addWidget(online_dot_);
  layout->addStretch();
  layout->addWidget(CreateToolButton(title_bar, QStringLiteral("\u8bed"),
                                     style()->standardIcon(QStyle::SP_MediaVolume)));
  layout->addWidget(CreateToolButton(title_bar, QStringLiteral("\u89c6"),
                                     style()->standardIcon(QStyle::SP_ComputerIcon)));
  layout->addWidget(CreateToolButton(title_bar, QStringLiteral("\u5c4f"),
                                     style()->standardIcon(QStyle::SP_DesktopIcon)));
  layout->addWidget(CreateToolButton(title_bar, QStringLiteral("\u63a7"),
                                     style()->standardIcon(QStyle::SP_DriveNetIcon)));
  layout->addWidget(CreateToolButton(title_bar, "+"));
  layout->addWidget(CreateToolButton(title_bar, "..."));

  return title_bar;
}

QWidget *ChatWindow::CreateMessageArea() {
  message_scroll_area_ = new QScrollArea(this);
  auto *container = new QWidget(message_scroll_area_);
  message_layout_ = new QVBoxLayout(container);

  message_scroll_area_->setObjectName("messageScrollArea");
  message_scroll_area_->setWidgetResizable(true);
  message_scroll_area_->setFrameShape(QFrame::NoFrame);
  message_scroll_area_->setWidget(container);

  container->setObjectName("messageContainer");
  message_layout_->setContentsMargins(0, 12, 0, 12);
  message_layout_->setSpacing(4);
  message_layout_->addStretch();

  return message_scroll_area_;
}

QWidget *ChatWindow::CreateInputArea() {
  auto *input_area = new QWidget(this);
  auto *root_layout = new QVBoxLayout(input_area);
  auto *tool_bar = new QWidget(input_area);
  auto *tool_layout = new QHBoxLayout(tool_bar);
  auto *send_row = new QWidget(input_area);
  auto *send_layout = new QHBoxLayout(send_row);
  auto *send_more_button = new QToolButton(send_row);

  input_area->setObjectName("inputArea");
  tool_bar->setObjectName("inputToolBar");
  send_more_button->setObjectName("sendMoreButton");
  send_more_button->setText("v");

  send_button_ = new QPushButton(QStringLiteral("\u53d1\u9001"), send_row);
  send_button_->setObjectName("sendButton");
  message_input_ = new QTextEdit(input_area);
  message_input_->setObjectName("messageInput");

  tool_layout->setContentsMargins(14, 0, 14, 0);
  tool_layout->setSpacing(6);
  tool_layout->addWidget(CreateToolButton(tool_bar, QStringLiteral("\u263a")));
  tool_layout->addWidget(CreateToolButton(tool_bar, QStringLiteral("\u2702")));
  tool_layout->addWidget(CreateToolButton(tool_bar, QStringLiteral("\u6587")));
  tool_layout->addWidget(CreateToolButton(tool_bar, QStringLiteral("\u56fe")));
  tool_layout->addWidget(CreateToolButton(tool_bar, QStringLiteral("\u5386")));
  tool_layout->addStretch();

  send_layout->setContentsMargins(0, 0, 14, 10);
  send_layout->setSpacing(6);
  send_layout->addStretch();
  send_layout->addWidget(send_button_);
  send_layout->addWidget(send_more_button);

  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);
  root_layout->addWidget(tool_bar);
  root_layout->addWidget(message_input_, 1);
  root_layout->addWidget(send_row);

  connect(send_button_, &QPushButton::clicked, this, &ChatWindow::HandleSend);
  return input_area;
}

void ChatWindow::AddTimestamp(const QString &time_text) {
  auto *timestamp = new QLabel(time_text, this);
  timestamp->setObjectName("messageTimestamp");
  timestamp->setAlignment(Qt::AlignCenter);
  message_layout_->insertWidget(message_layout_->count() - 1, timestamp);
}

void ChatWindow::AddMessage(const QString &text, bool sent_by_me) {
  message_layout_->insertWidget(message_layout_->count() - 1,
                                new MessageBubble(text, sent_by_me, this));
  ScrollToBottom();
}

void ChatWindow::FetchMessages(bool full_refresh) {
  if (current_target_user_id_ <= 0 || message_request_pending_) {
    return;
  }

  const int target_user_id = current_target_user_id_;
  message_request_pending_ = true;
  const QString path = QStringLiteral(
                           "/api/v1/messages?targetUserId=%1&page=1&pageSize=50")
                           .arg(target_user_id);
  ApiClient::instance()->get(
      path, this,
      [this, target_user_id, full_refresh](const QJsonObject &response) {
        message_request_pending_ = false;
        if (target_user_id != current_target_user_id_) {
          return;
        }
        if (full_refresh) {
          rendered_message_keys_.clear();
          last_rendered_minute_.clear();
          RemoveAllBubbles();
        }

        const QJsonArray messages =
            response.value("data").toObject().value("list").toArray();
        for (const QJsonValue &value : messages) {
          AppendMessage(value.toObject(), !full_refresh);
        }
        ScrollToBottom();
      },
      [this]() { message_request_pending_ = false; });
}

void ChatWindow::AppendMessage(const QJsonObject &message,
                               bool notify_new_message) {
  const QString key = MessageKey(message);
  if (rendered_message_keys_.contains(key)) {
    return;
  }

  const QString created_at = message.value("createdAt").toString();
  const QString minute = FormatTime(created_at);
  if (!minute.isEmpty() && minute != last_rendered_minute_) {
    AddTimestamp(minute);
    last_rendered_minute_ = minute;
  }

  const QString content = message.value("content").toString();
  const bool is_self = message.value("isSelf").toBool();
  rendered_message_keys_.insert(key);
  AddMessage(content, is_self);
  if (notify_new_message && !is_self) {
    emit messageReceived(current_target_user_id_, content, created_at);
  }
}

QString ChatWindow::MessageKey(const QJsonObject &message) const {
  const int message_id = message.value("messageId").toInt();
  if (message_id > 0) {
    return QStringLiteral("id:%1").arg(message_id);
  }
  return QStringLiteral("fallback:%1:%2:%3")
      .arg(message.value("createdAt").toString(),
           message.value("isSelf").toBool() ? QStringLiteral("1")
                                            : QStringLiteral("0"),
           message.value("content").toString());
}

void ChatWindow::RemoveAllBubbles() {
  while (message_layout_->count() > 1) {
    QLayoutItem *item = message_layout_->takeAt(0);
    if (item->widget() != nullptr) {
      item->widget()->deleteLater();
    }
    delete item;
  }
}

void ChatWindow::HandleSend() {
  const QString text = message_input_->toPlainText().trimmed();
  if (text.isEmpty() || current_target_user_id_ <= 0) {
    return;
  }

  send_button_->setEnabled(false);
  QJsonObject body;
  body.insert("receiverId", current_target_user_id_);
  body.insert("content", text);
  body.insert("type", 0);

  ApiClient::instance()->post(
      "/api/v1/messages", body, this,
      [this, text](const QJsonObject &response) {
        const QString created_at =
            response.value("data").toObject().value("createdAt").toString();
        QJsonObject message;
        message.insert("messageId",
                       response.value("data").toObject().value("messageId"));
        message.insert("content", text);
        message.insert("createdAt", created_at);
        message.insert("isSelf", true);
        AppendMessage(message, false);
        message_input_->clear();
        send_button_->setEnabled(true);
        emit messageSent(current_target_user_id_, text, created_at);
      },
      [this]() {
        send_button_->setEnabled(true);
        QMessageBox::warning(this, QString(), SendFailedText());
      });
}

void ChatWindow::RefreshCurrentMessages() {
  FetchMessages(false);
}

void ChatWindow::ScrollToBottom() {
  QTimer::singleShot(0, this, [this]() {
    message_scroll_area_->verticalScrollBar()->setValue(
        message_scroll_area_->verticalScrollBar()->maximum());
  });
}

QString ChatWindow::FormatTime(const QString &raw_time) const {
  const QDateTime date_time =
      QDateTime::fromString(raw_time, "yyyy-MM-dd HH:mm:ss");
  if (date_time.isValid()) {
    return date_time.time().toString("HH:mm");
  }
  return raw_time.left(5);
}
