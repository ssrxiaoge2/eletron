#pragma once

#include <QtWidgets/QWidget>

class QScrollArea;
class QLabel;
class QPushButton;
class QTextEdit;
class QVBoxLayout;

class ChatWindow : public QWidget {
  Q_OBJECT

 public:
  explicit ChatWindow(QWidget *parent = nullptr);
  void loadMessages(int target_user_id, const QString &target_username,
                    bool is_online);

 signals:
  void messageSent(int target_user_id, const QString &content,
                   const QString &created_at);

 private:
  QWidget *CreateTitleBar();
  QWidget *CreateMessageArea();
  QWidget *CreateInputArea();
  void AddTimestamp(const QString &time_text);
  void AddMessage(const QString &text, bool sent_by_me);
  void RemoveAllBubbles();
  void HandleSend();
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
};
