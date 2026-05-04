#include "ChatWindow.h"

#include "ApiClient.h"
#include "ChatInputEdit.h"
#include "MessageBubble.h"
#include "../widgets/ImageViewerDialog.h"
#include "../widgets/UploadProgressWidget.h"

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QUuid>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QtGlobal>
#include <QtGui/QResizeEvent>
#include <QtGui/QPixmap>
#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSizePolicy>
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

QString SendFailedText() {
  return QStringLiteral("\u53d1\u9001\u5931\u8d25\uff0c\u8bf7\u91cd\u8bd5");
}

QString UploadFailedText() {
  return QStringLiteral("\u4e0a\u4f20\u5931\u8d25\uff0c\u8bf7\u91cd\u8bd5");
}

QString DownloadFailedText() {
  return QStringLiteral("\u4e0b\u8f7d\u5931\u8d25\uff0c\u8bf7\u91cd\u8bd5");
}

QString OnlineDotStyle(bool is_online) {
  return QStringLiteral("border-radius: 4px; background-color: %1;")
      .arg(is_online ? QStringLiteral("#26c35a") : QStringLiteral("#9a9a9a"));
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

bool IsImageFileName(const QString &file_name) {
  const QString suffix = QFileInfo(file_name).suffix().toLower();
  return suffix == "png" || suffix == "jpg" || suffix == "jpeg" ||
         suffix == "gif" || suffix == "bmp" || suffix == "webp";
}

QJsonObject ParseContentJson(const QString &content) {
  const QJsonDocument document = QJsonDocument::fromJson(content.toUtf8());
  return document.isObject() ? document.object() : QJsonObject();
}

int ResolveMessageType(const QJsonObject &message) {
  const int raw_type = message.value("type").toInt(-1);
  if (raw_type == 1 || raw_type == 2) {
    return raw_type;
  }

  const QJsonObject content = ParseContentJson(message.value("content").toString());
  if (!content.isEmpty()) {
    const int file_type = content.value("fileType").toInt(
        content.value("type").toInt(-1));
    if (file_type == 1 || file_type == 2) {
      return file_type;
    }
    const QString file_name = content.value("fileName").toString();
    if (!file_name.isEmpty()) {
      return IsImageFileName(file_name) ? 1 : 2;
    }
    if (content.contains("fileId") || content.contains("file_id") ||
        content.contains("id")) {
      return 2;
    }
  }

  return 0;
}

}  // namespace

ChatWindow::ChatWindow(QWidget *parent) : QWidget(parent) {
  setObjectName("chatWindow");
  setMinimumWidth(300);

  auto *root_layout = new QVBoxLayout(this);
  message_refresh_timer_ = new QTimer(this);
  upload_progress_widget_ = new UploadProgressWidget(this);
  message_refresh_timer_->setInterval(2000);

  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);
  root_layout->addWidget(CreateTitleBar());
  root_layout->addWidget(CreateMessageArea(), 1);
  root_layout->addWidget(upload_progress_widget_);
  root_layout->addWidget(CreateInputArea());

  connect(message_refresh_timer_, &QTimer::timeout, this,
          &ChatWindow::RefreshCurrentMessages);
  connect(upload_progress_widget_, &UploadProgressWidget::cancelRequested, this,
          [this]() {
            if (current_upload_reply_ != nullptr) {
              upload_cancelled_ = true;
              current_upload_reply_->abort();
            }
            upload_progress_widget_->Finish();
          });
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
  message_scroll_area_->setAttribute(Qt::WA_StyledBackground, true);
  message_scroll_area_->setWidgetResizable(true);
  message_scroll_area_->setFrameShape(QFrame::NoFrame);
  message_scroll_area_->viewport()->setObjectName("messageViewport");
  message_scroll_area_->viewport()->setAttribute(Qt::WA_StyledBackground, true);
  message_scroll_area_->setStyleSheet(
      "QScrollArea { background: #ffffff; border: none; }");
  message_scroll_area_->viewport()->setStyleSheet(
      "QWidget#messageViewport { background-color: #ffffff; }");
  message_scroll_area_->setWidget(container);

  container->setObjectName("messageContainer");
  container->setAttribute(Qt::WA_StyledBackground, true);
  container->setStyleSheet(
      "QWidget#messageContainer { background-color: #ffffff; }");
  message_layout_->setContentsMargins(0, 12, 0, 16);
  message_layout_->setSpacing(2);
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
  message_input_ = new ChatInputEdit(input_area);
  message_input_->setObjectName("messageInput");

  tool_layout->setContentsMargins(14, 0, 14, 0);
  tool_layout->setSpacing(6);
  file_button_ = CreateToolButton(tool_bar, QStringLiteral("\u6587"));
  image_button_ = CreateToolButton(tool_bar, QStringLiteral("\u56fe"));

  tool_layout->addWidget(CreateToolButton(tool_bar, QStringLiteral("\u263a")));
  tool_layout->addWidget(CreateToolButton(tool_bar, QStringLiteral("\u2702")));
  tool_layout->addWidget(file_button_);
  tool_layout->addWidget(image_button_);
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
  connect(image_button_, &QToolButton::clicked, this,
          &ChatWindow::HandleSelectImage);
  connect(file_button_, &QToolButton::clicked, this,
          &ChatWindow::HandleSelectFile);
  connect(message_input_, &ChatInputEdit::imageDropped, this,
          &ChatWindow::SendImageFile);
  connect(message_input_, &ChatInputEdit::imagePasted, this,
          &ChatWindow::SendPastedImage);
  return input_area;
}

void ChatWindow::AddTimestamp(const QString &time_text) {
  auto *time_row = new QWidget(this);
  auto *time_layout = new QHBoxLayout(time_row);
  auto *timestamp = new QLabel(time_text, time_row);

  time_row->setObjectName("timestampRow");
  time_row->setAttribute(Qt::WA_TranslucentBackground, true);
  time_row->setStyleSheet(
      "QWidget#timestampRow { background: transparent; }");
  time_row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  timestamp->setObjectName("timestampLabel");
  timestamp->setAttribute(Qt::WA_TranslucentBackground, true);
  timestamp->setAlignment(Qt::AlignHCenter);
  timestamp->setMinimumWidth(0);
  timestamp->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  time_layout->setContentsMargins(0, 4, 0, 4);
  time_layout->setSpacing(0);
  time_layout->addStretch();
  time_layout->addWidget(timestamp);
  time_layout->addStretch();

  message_layout_->insertWidget(message_layout_->count() - 1, time_row);
}

void ChatWindow::AddMessage(const QString &content,
                            MessageBubble::BubbleType type, bool sent_by_me,
                            const QString &time) {
  auto *bubble = new MessageBubble(content, type, sent_by_me, time, this);
  bubble->setMaxBubbleWidth(MaxBubbleWidth());
  bubbles_.append(bubble);
  connect(bubble, &MessageBubble::imageOpenRequested, this,
          &ChatWindow::OpenImage);
  connect(bubble, &MessageBubble::fileDownloadRequested, this,
          &ChatWindow::DownloadFile);
  message_layout_->insertWidget(message_layout_->count() - 1, bubble);
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
  const QString fallback_key = FallbackMessageKey(message);
  if (rendered_message_keys_.contains(key) ||
      rendered_message_keys_.contains(fallback_key)) {
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
  const int type = ResolveMessageType(message);
  qDebug() << "消息type:" << type
           << "content:" << content.left(50);
  rendered_message_keys_.insert(key);
  rendered_message_keys_.insert(fallback_key);
  AddMessage(content, static_cast<MessageBubble::BubbleType>(type), is_self,
             created_at);
  if (notify_new_message && !is_self) {
    emit messageReceived(current_target_user_id_, content, created_at);
  }
}

QString ChatWindow::MessageKey(const QJsonObject &message) const {
  const int message_id = message.value("messageId").toInt();
  if (message_id > 0) {
    return QStringLiteral("id:%1").arg(message_id);
  }
  return FallbackMessageKey(message);
}

QString ChatWindow::FallbackMessageKey(const QJsonObject &message) const {
  return QStringLiteral("fallback:%1:%2:%3")
      .arg(message.value("createdAt").toString(),
           message.value("isSelf").toBool() ? QStringLiteral("1")
                                            : QStringLiteral("0"),
           QStringLiteral("%1:%2")
               .arg(message.value("type").toInt(0))
               .arg(message.value("content").toString()));
}

void ChatWindow::RemoveAllBubbles() {
  bubbles_.clear();
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
        message.insert("type", 0);
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

void ChatWindow::HandleSelectImage() {
  if (current_target_user_id_ <= 0) {
    return;
  }
  const QString file_path = QFileDialog::getOpenFileName(
      this, QStringLiteral("\u9009\u62e9\u56fe\u7247"), QString(),
      QStringLiteral("\u56fe\u7247\u6587\u4ef6 (*.png *.jpg *.jpeg *.gif *.bmp *.webp)"));
  if (file_path.isEmpty()) {
    return;
  }
  SendImageFile(file_path);
}

void ChatWindow::HandleSelectFile() {
  if (current_target_user_id_ <= 0) {
    return;
  }
  const QString file_path = QFileDialog::getOpenFileName(
      this, QStringLiteral("\u9009\u62e9\u6587\u4ef6"), QString(),
      QStringLiteral("\u6240\u6709\u6587\u4ef6 (*.*)"));
  if (file_path.isEmpty()) {
    return;
  }
  UploadAttachment(file_path, 2, 100LL * 1024 * 1024,
                   QStringLiteral("\u6587\u4ef6\u5927\u5c0f\u4e0d\u80fd\u8d85\u8fc7 100MB"));
}

void ChatWindow::SendImageFile(const QString &file_path) {
  UploadAttachment(file_path, 1, 20LL * 1024 * 1024,
                   QStringLiteral("\u56fe\u7247\u5927\u5c0f\u4e0d\u80fd\u8d85\u8fc7 20MB"));
}

void ChatWindow::SendPastedImage(const QPixmap &pixmap) {
  if (pixmap.isNull()) {
    return;
  }

  QString temp_dir =
      QStandardPaths::writableLocation(QStandardPaths::TempLocation);
  if (temp_dir.isEmpty()) {
    temp_dir = QDir::tempPath();
  }
  const QString file_path =
      QDir(temp_dir).filePath(QStringLiteral("feige_paste_%1.png")
                                  .arg(QUuid::createUuid().toString(QUuid::Id128)));
  if (!pixmap.save(file_path, "PNG")) {
    QMessageBox::warning(this, QStringLiteral("\u63d0\u793a"),
                         QStringLiteral("\u7c98\u8d34\u56fe\u7247\u4fdd\u5b58\u5931\u8d25"));
    return;
  }
  SendImageFile(file_path);
}

void ChatWindow::UploadAttachment(const QString &file_path, int type,
                                  qint64 max_size,
                                  const QString &too_large_message) {
  const QFileInfo file_info(file_path);
  if (!file_info.exists() || !file_info.isFile()) {
    return;
  }
  if (file_info.size() > max_size) {
    QMessageBox::warning(this, QStringLiteral("\u63d0\u793a"),
                         too_large_message);
    return;
  }

  upload_progress_widget_->Start(file_info.fileName());
  upload_cancelled_ = false;
  current_upload_reply_ = ApiClient::instance()->uploadFile(
      file_path, type, current_target_user_id_, this,
      [this, type](const QJsonObject &response) {
        current_upload_reply_ = nullptr;
        upload_progress_widget_->Finish();
        const QJsonObject data = response.value("data").toObject();
        FetchMessages(false);
        emit messageSent(current_target_user_id_,
                         type == 1 ? QStringLiteral("[\u56fe\u7247]")
                                   : QStringLiteral("[\u6587\u4ef6]"),
                         data.value("createdAt").toString(
                             QDateTime::currentDateTime().toString(
                                 "yyyy-MM-dd HH:mm:ss")));
      },
      [this]() {
        current_upload_reply_ = nullptr;
        upload_progress_widget_->Finish();
        if (upload_cancelled_) {
          upload_cancelled_ = false;
          return;
        }
        QMessageBox::warning(this, QStringLiteral("\u63d0\u793a"),
                             UploadFailedText());
      },
      [this](qint64 sent, qint64 total) {
        upload_progress_widget_->SetProgress(sent, total);
      });
  if (current_upload_reply_ == nullptr) {
    upload_progress_widget_->Finish();
  }
}

void ChatWindow::OpenImage(int file_id, const QString &file_name,
                           qint64 file_size) {
  ApiClient::instance()->downloadBytes(
      QStringLiteral("/api/v1/files/download/%1").arg(file_id), this,
      [this, file_name, file_size](const QByteArray &data) {
        QPixmap pixmap;
        if (!pixmap.loadFromData(data)) {
          QMessageBox::warning(this, QStringLiteral("\u63d0\u793a"),
                               DownloadFailedText());
          return;
        }
        auto *dialog = new ImageViewerDialog(pixmap, file_name, file_size, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
      },
      [this]() {
        QMessageBox::warning(this, QStringLiteral("\u63d0\u793a"),
                             DownloadFailedText());
      });
}

void ChatWindow::DownloadFile(int file_id, const QString &file_name,
                              qint64 file_size) {
  const QString readable_size = ReadableFileSize(file_size);
  const QMessageBox::StandardButton choice = QMessageBox::question(
      this, QStringLiteral("\u4e0b\u8f7d\u6587\u4ef6"),
      QStringLiteral("\u662f\u5426\u4e0b\u8f7d\uff1a%1\uff08%2\uff09\uff1f")
          .arg(file_name, readable_size));
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
  progress->setWindowModality(Qt::WindowModal);
  progress->setAttribute(Qt::WA_DeleteOnClose);
  QNetworkReply *reply = ApiClient::instance()->downloadBytes(
      QStringLiteral("/api/v1/files/download/%1").arg(file_id), this,
      [this, save_path, progress](const QByteArray &data) {
        progress->close();
        QFile file(save_path);
        if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size()) {
          QMessageBox::warning(this, QStringLiteral("\u63d0\u793a"),
                               DownloadFailedText());
          return;
        }
        QMessageBox::information(
            this, QStringLiteral("\u4e0b\u8f7d\u5b8c\u6210"),
            QStringLiteral("\u6587\u4ef6\u5df2\u4fdd\u5b58\u81f3\uff1a%1")
                .arg(save_path));
      },
      [this, progress]() {
        progress->close();
        QMessageBox::warning(this, QStringLiteral("\u63d0\u793a"),
                             DownloadFailedText());
      },
      [progress](qint64 received, qint64 total) {
        if (total > 0) {
          progress->setValue(static_cast<int>(received * 100 / total));
        }
      });
  connect(progress, &QProgressDialog::canceled, reply, &QNetworkReply::abort);
  progress->show();
}

void ChatWindow::RefreshCurrentMessages() {
  FetchMessages(false);
}

void ChatWindow::ScrollToBottom() {
  auto scroll_to_bottom = [this]() {
    message_scroll_area_->verticalScrollBar()->setValue(
        message_scroll_area_->verticalScrollBar()->maximum());
  };
  QTimer::singleShot(0, this, scroll_to_bottom);
  QTimer::singleShot(50, this, scroll_to_bottom);
  QTimer::singleShot(150, this, scroll_to_bottom);
}

int ChatWindow::MaxBubbleWidth() const {
  const int width = message_scroll_area_ == nullptr
                        ? this->width()
                        : message_scroll_area_->viewport()->width();
  return qMax(160, width * 65 / 100);
}

void ChatWindow::UpdateBubbleWidths() {
  const int max_width = MaxBubbleWidth();
  for (MessageBubble *bubble : bubbles_) {
    if (bubble != nullptr) {
      bubble->setMaxBubbleWidth(max_width);
    }
  }
}

void ChatWindow::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  UpdateBubbleWidths();
}

QString ChatWindow::FormatTime(const QString &raw_time) const {
  const QDateTime date_time =
      QDateTime::fromString(raw_time, "yyyy-MM-dd HH:mm:ss");
  if (date_time.isValid()) {
    return date_time.time().toString("HH:mm");
  }
  return raw_time.left(5);
}
