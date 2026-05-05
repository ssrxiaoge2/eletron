#include "GroupChatWindow.h"

#include "GroupAnnouncementWidget.h"
#include "GroupMemberWidget.h"
#include "GroupMessageBubble.h"
#include "../src/ApiClient.h"
#include "../src/ChatInputEdit.h"
#include "ImageViewerDialog.h"

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtGui/QResizeEvent>
#include <QtGui/QPixmap>
#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStyle>
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

QJsonObject ParseContentJson(const QString &content) {
  const QJsonDocument document = QJsonDocument::fromJson(content.toUtf8());
  return document.isObject() ? document.object() : QJsonObject();
}

QString ReadableFileSize(qint64 size) {
  if (size < 1024) {
    return QStringLiteral("%1 B").arg(size);
  }
  if (size < 1024 * 1024) {
    return QStringLiteral("%1 KB").arg(size / 1024.0, 0, 'f', 1);
  }
  return QStringLiteral("%1 MB").arg(size / 1024.0 / 1024.0, 0, 'f', 1);
}

}  // namespace

GroupChatWindow::GroupChatWindow(QWidget *parent) : QWidget(parent) {
  setObjectName("groupChatWindow");
  setMinimumWidth(300);
  auto *root_layout = new QVBoxLayout(this);
  auto *content = new QWidget(this);
  auto *content_layout = new QHBoxLayout(content);
  auto *left_panel = new QWidget(content);
  auto *left_layout = new QVBoxLayout(left_panel);

  message_timer_ = new QTimer(this);
  message_timer_->setInterval(30000);

  left_layout->setContentsMargins(0, 0, 0, 0);
  left_layout->setSpacing(0);
  left_layout->addWidget(CreateMessageArea(), 1);
  left_layout->addWidget(CreateInputArea());

  content_layout->setContentsMargins(0, 0, 0, 0);
  content_layout->setSpacing(0);
  content_layout->addWidget(left_panel, 1);
  content_layout->addWidget(CreateSidePanel());

  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);
  root_layout->addWidget(CreateTitleBar());
  root_layout->addWidget(content, 1);

  connect(message_timer_, &QTimer::timeout, this,
          [this]() { LoadMessages(false); });
  LoadProfile();
}

void GroupChatWindow::openGroup(int group_id, const QString &group_name,
                                int member_count, int my_role) {
  group_id_ = group_id;
  group_name_ = group_name;
  member_count_ = member_count;
  my_role_ = my_role;
  title_label_->setText(QStringLiteral("%1 (%2)").arg(group_name_).arg(member_count_));
  announcement_widget_->setGroup(group_id_, my_role_ == 2);
  member_widget_->setGroup(group_id_, my_role_, current_user_id_);
  last_message_id_ = 0;
  rendered_message_ids_.clear();
  last_rendered_minute_.clear();
  RemoveAllMessages();
  LoadGroupInfo();
  LoadMessages(true);
  message_timer_->start();
}

QWidget *GroupChatWindow::CreateTitleBar() {
  auto *title_bar = new QWidget(this);
  auto *layout = new QHBoxLayout(title_bar);
  title_label_ = new QLabel(title_bar);
  auto *online_dot = new QLabel(title_bar);
  more_button_ = CreateToolButton(title_bar, "...");

  title_bar->setObjectName("chatTitleBar");
  title_label_->setObjectName("chatTitleName");
  online_dot->setFixedSize(8, 8);
  online_dot->setStyleSheet("border-radius: 4px; background-color: #26c35a;");
  online_dot->setAttribute(Qt::WA_StyledBackground, true);

  layout->setContentsMargins(20, 0, 16, 0);
  layout->setSpacing(8);
  layout->addWidget(title_label_);
  layout->addWidget(online_dot);
  layout->addStretch();
  layout->addWidget(CreateToolButton(title_bar, QStringLiteral("\u8bed"),
                                     style()->standardIcon(QStyle::SP_MediaVolume)));
  layout->addWidget(CreateToolButton(title_bar, QStringLiteral("\u89c6"),
                                     style()->standardIcon(QStyle::SP_ComputerIcon)));
  layout->addWidget(CreateToolButton(title_bar, "+"));
  layout->addWidget(more_button_);

  connect(more_button_, &QToolButton::clicked, this,
          &GroupChatWindow::ShowMoreMenu);
  return title_bar;
}

QWidget *GroupChatWindow::CreateMessageArea() {
  message_scroll_area_ = new QScrollArea(this);
  auto *container = new QWidget(message_scroll_area_);
  message_layout_ = new QVBoxLayout(container);

  message_scroll_area_->setObjectName("messageScrollArea");
  message_scroll_area_->setWidgetResizable(true);
  message_scroll_area_->setFrameShape(QFrame::NoFrame);
  message_scroll_area_->viewport()->setStyleSheet("background-color: #ffffff;");
  message_scroll_area_->setStyleSheet(
      "QScrollArea { background: #ffffff; border: none; }");
  container->setObjectName("messageContainer");
  container->setStyleSheet("QWidget#messageContainer { background: #ffffff; }");
  message_layout_->setContentsMargins(0, 12, 0, 16);
  message_layout_->setSpacing(2);
  message_layout_->addStretch();
  message_scroll_area_->setWidget(container);
  return message_scroll_area_;
}

QWidget *GroupChatWindow::CreateInputArea() {
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
  message_input_ = new ChatInputEdit(input_area);
  message_input_->setObjectName("messageInput");
  send_button_ = new QPushButton(QStringLiteral("\u53d1\u9001"), send_row);
  send_button_->setObjectName("sendButton");

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
  connect(send_button_, &QPushButton::clicked, this, &GroupChatWindow::SendText);
  return input_area;
}

QWidget *GroupChatWindow::CreateSidePanel() {
  auto *panel = new QWidget(this);
  auto *layout = new QVBoxLayout(panel);
  announcement_widget_ = new GroupAnnouncementWidget(panel);
  member_widget_ = new GroupMemberWidget(panel);
  panel->setObjectName("groupSidePanel");
  panel->setFixedWidth(200);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(announcement_widget_);
  layout->addWidget(member_widget_, 1);
  connect(member_widget_, &GroupMemberWidget::membersChanged, this,
          [this](int member_count) {
            member_count_ = member_count;
            title_label_->setText(QStringLiteral("%1 (%2)")
                                      .arg(group_name_)
                                      .arg(member_count_));
          });
  return panel;
}

void GroupChatWindow::LoadProfile() {
  ApiClient::instance()->get("/api/v1/user/profile", this,
                             [this](const QJsonObject &response) {
                               current_user_id_ =
                                   response.value("data").toObject().value("userId").toInt();
                               if (group_id_ > 0) {
                                 member_widget_->setGroup(group_id_, my_role_,
                                                          current_user_id_);
                               }
                             });
}

void GroupChatWindow::LoadGroupInfo() {
  ApiClient::instance()->get(
      QStringLiteral("/api/v1/groups/%1").arg(group_id_), this,
      [this](const QJsonObject &response) {
        const QJsonObject data = response.value("data").toObject();
        group_name_ = data.value("name").toString(group_name_);
        member_count_ = data.value("memberCount").toInt(member_count_);
        title_label_->setText(QStringLiteral("%1 (%2)").arg(group_name_).arg(member_count_));
      });
}

void GroupChatWindow::LoadMessages(bool full_refresh) {
  if (group_id_ <= 0 || message_request_pending_) {
    return;
  }
  message_request_pending_ = true;
  ApiClient::instance()->get(
      QStringLiteral("/api/v1/groups/%1/messages?page=1&pageSize=80").arg(group_id_),
      this,
      [this, full_refresh](const QJsonObject &response) {
        message_request_pending_ = false;
        if (full_refresh) {
          last_message_id_ = 0;
          rendered_message_ids_.clear();
          last_rendered_minute_.clear();
          RemoveAllMessages();
        }
        const QJsonArray messages =
            response.value("data").toObject().value("list").toArray();
        for (const QJsonValue &value : messages) {
          AppendMessage(value.toObject());
        }
        ScrollToBottom();
      },
      [this]() { message_request_pending_ = false; });
}

void GroupChatWindow::AppendMessage(const QJsonObject &message) {
  const int message_id = message.value("messageId").toInt();
  if (message_id > 0 && rendered_message_ids_.contains(message_id)) {
    return;
  }
  if (message_id > 0 && message_id < last_message_id_) {
    return;
  }

  const QString created_at = message.value("createdAt").toString();
  const QString minute = FormatTime(created_at);
  if (!minute.isEmpty() && minute != last_rendered_minute_) {
    AddTimestamp(minute);
    last_rendered_minute_ = minute;
  }

  auto *bubble = new GroupMessageBubble(
      message.value("content").toString(),
      static_cast<MessageBubble::BubbleType>(ResolveMessageType(message)),
      message.value("isSelf").toBool(),
      message.value("senderNickname").toString(), message.value("senderRole").toInt(),
      created_at, this);
  bubble->setMaxBubbleWidth(MaxBubbleWidth());
  connect(bubble, &GroupMessageBubble::imageOpenRequested, this,
          &GroupChatWindow::OpenImage);
  connect(bubble, &GroupMessageBubble::fileDownloadRequested, this,
          &GroupChatWindow::DownloadFile);
  bubbles_.append(bubble);
  message_layout_->insertWidget(message_layout_->count() - 1, bubble);
  if (message_id > 0) {
    rendered_message_ids_.insert(message_id);
    last_message_id_ = qMax(last_message_id_, message_id);
  }
}

void GroupChatWindow::AddTimestamp(const QString &time_text) {
  auto *time_row = new QWidget(this);
  auto *time_layout = new QHBoxLayout(time_row);
  auto *timestamp = new QLabel(time_text, time_row);
  time_row->setStyleSheet("background: transparent;");
  timestamp->setObjectName("timestampLabel");
  timestamp->setAlignment(Qt::AlignHCenter);
  timestamp->setMinimumWidth(0);
  timestamp->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  time_layout->setContentsMargins(0, 4, 0, 4);
  time_layout->addStretch();
  time_layout->addWidget(timestamp);
  time_layout->addStretch();
  message_layout_->insertWidget(message_layout_->count() - 1, time_row);
}

void GroupChatWindow::RemoveAllMessages() {
  bubbles_.clear();
  while (message_layout_->count() > 1) {
    QLayoutItem *item = message_layout_->takeAt(0);
    if (item->widget() != nullptr) {
      item->widget()->deleteLater();
    }
    delete item;
  }
}

void GroupChatWindow::SendText() {
  const QString text = message_input_->toPlainText().trimmed();
  if (text.isEmpty() || group_id_ <= 0) {
    return;
  }
  send_button_->setEnabled(false);
  QJsonObject body;
  body.insert("content", text);
  body.insert("type", 0);
  ApiClient::instance()->post(
      QStringLiteral("/api/v1/groups/%1/messages").arg(group_id_), body, this,
      [this, text](const QJsonObject &response) {
        qDebug() << "\u53d1\u9001\u7fa4\u6d88\u606f\u54cd\u5e94\uff1a"
                 << QJsonDocument(response).toJson(QJsonDocument::Compact);
        send_button_->setEnabled(true);
        message_input_->clear();
        const QJsonObject data = response.value("data").toObject();
        const QString created_at = data.value("createdAt").toString(
            QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        qDebug() << "\u5f00\u59cb\u8ffd\u52a0\u7fa4\u6d88\u606f\u6c14\u6ce1\uff0ccontent\uff1a"
                 << text;
        QJsonObject message;
        message.insert("messageId",
                       data.value("messageId").toInt(
                           -static_cast<int>(
                               QDateTime::currentMSecsSinceEpoch() %
                               1000000000)));
        message.insert("senderId", current_user_id_);
        message.insert("senderNickname", QString());
        message.insert("senderRole", my_role_);
        message.insert("content", text);
        message.insert("type", 0);
        message.insert("createdAt", created_at);
        message.insert("isSelf", true);
        AppendMessage(message);
        message_scroll_area_->widget()->adjustSize();
        message_scroll_area_->widget()->updateGeometry();
        ScrollToBottom();
        QApplication::processEvents();
        emit groupMessageSent(group_id_, text, created_at);
        LoadMessages(false);
      },
      [this]() {
        send_button_->setEnabled(true);
        QMessageBox::warning(
            this, QString(),
            QStringLiteral("\u60a8\u5df2\u88ab\u7981\u8a00\uff0c\u6216\u7fa4\u6d88\u606f\u53d1\u9001\u5931\u8d25"));
      });
}

void GroupChatWindow::ShowMoreMenu() {
  QMenu menu(this);
  QAction *rename_action = nullptr;
  QAction *announcement_action = nullptr;
  QAction *dissolve_action = nullptr;
  if (my_role_ == 2) {
    rename_action = menu.addAction(QStringLiteral("\u4fee\u6539\u7fa4\u540d\u79f0"));
    announcement_action =
        menu.addAction(QStringLiteral("\u53d1\u5e03\u7fa4\u516c\u544a"));
    dissolve_action = menu.addAction(QStringLiteral("\u89e3\u6563\u8be5\u7fa4"));
  } else {
    menu.addAction(QStringLiteral("\u66f4\u591a"));
  }
  QAction *selected = menu.exec(
      more_button_->mapToGlobal(QPoint(0, more_button_->height())));
  if (selected == rename_action) {
    RenameGroup();
  } else if (selected == announcement_action) {
    announcement_widget_->editAnnouncement();
  } else if (selected == dissolve_action) {
    DissolveGroup();
  }
}

void GroupChatWindow::RenameGroup() {
  bool ok = false;
  const QString name = QInputDialog::getText(
      this, QStringLiteral("\u4fee\u6539\u7fa4\u540d\u79f0"),
      QStringLiteral("\u7fa4\u540d\u79f0\uff1a"), QLineEdit::Normal,
      group_name_, &ok);
  if (!ok || name.trimmed().isEmpty()) {
    return;
  }
  QJsonObject body;
  body.insert("name", name.trimmed());
  ApiClient::instance()->put(
      QStringLiteral("/api/v1/groups/%1/name").arg(group_id_), body, this,
      [this, name](const QJsonObject &) {
        group_name_ = name.trimmed();
        title_label_->setText(QStringLiteral("%1 (%2)")
                                  .arg(group_name_)
                                  .arg(member_count_));
      },
      [this]() {
        QMessageBox::warning(this, QString(),
                             QStringLiteral("\u4fee\u6539\u7fa4\u540d\u5931\u8d25"));
      });
}

void GroupChatWindow::DissolveGroup() {
  const QMessageBox::StandardButton choice = QMessageBox::question(
      this, QStringLiteral("\u89e3\u6563\u7fa4\u804a"),
      QStringLiteral("\u89e3\u6563\u540e\u6240\u6709\u6210\u5458\u5c06\u88ab\u79fb\u51fa\u7fa4\u804a\uff0c\u4e14\u65e0\u6cd5\u6062\u590d\uff0c\u786e\u8ba4\u89e3\u6563\uff1f"));
  if (choice != QMessageBox::Yes) {
    return;
  }
  ApiClient::instance()->deleteResource(
      QStringLiteral("/api/v1/groups/%1").arg(group_id_), this,
      [this](const QJsonObject &) {
        message_timer_->stop();
        emit groupDeleted(group_id_);
      },
      [this]() {
        QMessageBox::warning(this, QString(),
                             QStringLiteral("\u89e3\u6563\u7fa4\u804a\u5931\u8d25"));
      });
}

void GroupChatWindow::ScrollToBottom() {
  auto scroll = [this]() {
    message_scroll_area_->verticalScrollBar()->setValue(
        message_scroll_area_->verticalScrollBar()->maximum());
  };
  QTimer::singleShot(0, this, scroll);
  QTimer::singleShot(80, this, scroll);
}

QString GroupChatWindow::FormatTime(const QString &raw_time) const {
  const QDateTime date_time =
      QDateTime::fromString(raw_time, "yyyy-MM-dd HH:mm:ss");
  return date_time.isValid() ? date_time.time().toString("HH:mm")
                             : raw_time.left(5);
}

int GroupChatWindow::ResolveMessageType(const QJsonObject &message) const {
  const int raw_type = message.value("type").toInt(0);
  if (raw_type == 1 || raw_type == 2) {
    return raw_type;
  }
  const QJsonObject content =
      ParseContentJson(message.value("content").toString());
  if (content.contains("fileId")) {
    return content.value("fileType").toInt(2);
  }
  return 0;
}

int GroupChatWindow::MaxBubbleWidth() const {
  const int width = message_scroll_area_ == nullptr
                        ? this->width()
                        : message_scroll_area_->viewport()->width();
  return qMax(160, width * 65 / 100);
}

void GroupChatWindow::UpdateBubbleWidths() {
  const int max_width = MaxBubbleWidth();
  for (GroupMessageBubble *bubble : bubbles_) {
    if (bubble != nullptr) {
      bubble->setMaxBubbleWidth(max_width);
    }
  }
}

void GroupChatWindow::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  UpdateBubbleWidths();
}

void GroupChatWindow::OpenImage(int file_id, const QString &file_name,
                                qint64 file_size) {
  ApiClient::instance()->downloadBytes(
      QStringLiteral("/api/v1/files/download/%1").arg(file_id), this,
      [this, file_name, file_size](const QByteArray &data) {
        QPixmap pixmap;
        if (!pixmap.loadFromData(data)) {
          QMessageBox::warning(this, QString(),
                               QStringLiteral("\u56fe\u7247\u6253\u5f00\u5931\u8d25"));
          return;
        }
        auto *dialog = new ImageViewerDialog(pixmap, file_name, file_size, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
      });
}

void GroupChatWindow::DownloadFile(int file_id, const QString &file_name,
                                   qint64 file_size) {
  const QMessageBox::StandardButton choice = QMessageBox::question(
      this, QStringLiteral("\u4e0b\u8f7d\u6587\u4ef6"),
      QStringLiteral("\u662f\u5426\u4e0b\u8f7d\uff1a%1\uff08%2\uff09\uff1f")
          .arg(file_name, ReadableFileSize(file_size)));
  if (choice != QMessageBox::Yes) {
    return;
  }
  const QString save_path = QFileDialog::getSaveFileName(
      this, QStringLiteral("\u4fdd\u5b58\u6587\u4ef6"), file_name);
  if (save_path.isEmpty()) {
    return;
  }
  auto *progress = new QProgressDialog(
      QStringLiteral("\u6b63\u5728\u4e0b\u8f7d\uff1a%1").arg(file_name),
      QStringLiteral("\u53d6\u6d88"), 0, 100, this);
  progress->setAttribute(Qt::WA_DeleteOnClose);
  QNetworkReply *reply = ApiClient::instance()->downloadBytes(
      QStringLiteral("/api/v1/files/download/%1").arg(file_id), this,
      [this, save_path, progress](const QByteArray &data) {
        progress->close();
        QFile file(save_path);
        if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size()) {
          QMessageBox::warning(this, QString(),
                               QStringLiteral("\u4e0b\u8f7d\u5931\u8d25"));
          return;
        }
        QMessageBox::information(this, QStringLiteral("\u4e0b\u8f7d\u5b8c\u6210"),
                                 QStringLiteral("\u6587\u4ef6\u5df2\u4fdd\u5b58\u81f3\uff1a%1")
                                     .arg(save_path));
      },
      [this, progress]() {
        progress->close();
        QMessageBox::warning(this, QString(),
                             QStringLiteral("\u4e0b\u8f7d\u5931\u8d25"));
      },
      [progress](qint64 received, qint64 total) {
        if (total > 0) {
          progress->setValue(static_cast<int>(received * 100 / total));
        }
      });
  connect(progress, &QProgressDialog::canceled, reply, &QNetworkReply::abort);
  progress->show();
}
