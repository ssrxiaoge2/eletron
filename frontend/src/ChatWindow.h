#pragma once

#include <QtWidgets/QWidget>

class QScrollArea;
class QTextEdit;
class QVBoxLayout;

class ChatWindow : public QWidget {
  Q_OBJECT

 public:
  explicit ChatWindow(QWidget *parent = nullptr);

 private:
  QWidget *CreateTitleBar();
  QWidget *CreateMessageArea();
  QWidget *CreateInputArea();
  void AddTimestamp(const QString &time_text);
  void AddMessage(const QString &text, bool sent_by_me);
  void HandleSend();
  void ScrollToBottom();

  QScrollArea *message_scroll_area_;
  QVBoxLayout *message_layout_;
  QTextEdit *message_input_;
};
