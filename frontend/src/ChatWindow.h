#pragma once

#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtWidgets/QWidget>

#include "MessageBubble.h"

class ChatInputEdit;
class QJsonObject;
class QScrollArea;
class QLabel;
class QNetworkReply;
class QPushButton;
class QPixmap;
class QResizeEvent;
class QToolButton;
class QTimer;
class QVBoxLayout;
class UploadProgressWidget;

class ChatWindow : public QWidget {
  Q_OBJECT

 public:
  explicit ChatWindow(QWidget *parent = nullptr);
  void loadMessages(int target_user_id, const QString &target_username,
                    bool is_online);
  void updateOnlineStatus(int target_user_id, bool is_online);

signals:
  void messageSent(int target_user_id, const QString &content,
                   const QString &created_at);
  void messageReceived(int target_user_id, const QString &content,
                       const QString &created_at);

 protected:
  void resizeEvent(QResizeEvent *event) override;

 private:
  QWidget *CreateTitleBar();
  QWidget *CreateMessageArea();
  QWidget *CreateInputArea();
  void AddTimestamp(const QString &time_text);
  void AddMessage(const QString &content, MessageBubble::BubbleType type,
                  bool sent_by_me, const QString &time = QString());
  void FetchMessages(bool full_refresh);
  void AppendMessage(const QJsonObject &message, bool notify_new_message);
  QString MessageKey(const QJsonObject &message) const;
  QString FallbackMessageKey(const QJsonObject &message) const;
  void RemoveAllBubbles();
  void HandleSend();
  void HandleSelectImage();
  void HandleSelectFile();
  void SendImageFile(const QString &file_path);
  void SendPastedImage(const QPixmap &pixmap);
  void UploadAttachment(const QString &file_path, int type, qint64 max_size,
                        const QString &too_large_message);
  void OpenImage(int file_id, const QString &file_name, qint64 file_size);
  void DownloadFile(int file_id, const QString &file_name, qint64 file_size);
  void RefreshCurrentMessages();
  void ScrollToBottom();
  QString FormatTime(const QString &raw_time) const;
  int MaxBubbleWidth() const;
  void UpdateBubbleWidths();

  int current_target_user_id_ = 0;
  QString current_target_username_;
  QScrollArea *message_scroll_area_;
  QVBoxLayout *message_layout_;
  ChatInputEdit *message_input_;
  QPushButton *send_button_;
  QToolButton *image_button_;
  QToolButton *file_button_;
  QLabel *name_label_;
  QLabel *online_dot_;
  QTimer *message_refresh_timer_;
  UploadProgressWidget *upload_progress_widget_;
  QNetworkReply *current_upload_reply_ = nullptr;
  bool upload_cancelled_ = false;
  QList<MessageBubble *> bubbles_;
  QSet<QString> rendered_message_keys_;
  QString last_rendered_minute_;
  bool message_request_pending_ = false;
};
