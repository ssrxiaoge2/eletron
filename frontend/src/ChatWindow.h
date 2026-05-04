#pragma once

#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtWidgets/QWidget>

class QJsonObject;
class QScrollArea;
class QLabel;
class QPushButton;
class QTextEdit;
class QTimer;
class QVBoxLayout;

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

 private:
  QWidget *CreateTitleBar();
  QWidget *CreateMessageArea();
  QWidget *CreateInputArea();
  void AddTimestamp(const QString &time_text);
  void AddMessage(const QString &text, bool sent_by_me);
  void FetchMessages(bool full_refresh);
  void AppendMessage(const QJsonObject &message, bool notify_new_message);
  QString MessageKey(const QJsonObject &message) const;
  void RemoveAllBubbles();
  void HandleSend();
  void RefreshCurrentMessages();
  void ScrollToBottom();
  QString FormatTime(const QString &raw_time) const;

  int current_target_user_id_ = 0;
  QString current_target_username_;
  QScrollArea *message_scroll_area_;
  QVBoxLayout *message_layout_;
  QTextEdit *message_input_;
  QPushButton *send_button_;
  QLabel *name_label_;
  QLabel *online_dot_;
  QTimer *message_refresh_timer_;
  QSet<QString> rendered_message_keys_;
  QString last_rendered_minute_;
  bool message_request_pending_ = false;
};
