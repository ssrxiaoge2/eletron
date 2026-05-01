#include "ChatWindow.h"

#include "MessageBubble.h"

#include <QtCore/QTimer>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFrame>
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

}  // namespace

ChatWindow::ChatWindow(QWidget *parent) : QWidget(parent) {
  setObjectName("chatWindow");

  auto *root_layout = new QVBoxLayout(this);
  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);
  root_layout->addWidget(CreateTitleBar());
  root_layout->addWidget(CreateMessageArea(), 1);
  root_layout->addWidget(CreateInputArea());

  AddTimestamp(QStringLiteral("15:27"));
  AddMessage(QStringLiteral("\u4f60\u597d\uff0cMainWindow \u7ec8\u4e8e"
                            "\u5f00\u59cb\u50cf\u4e00\u4e2a\u804a\u5929"
                            "\u7a97\u53e3\u4e86\u3002"),
             false);
  AddMessage(QStringLiteral("\u5148\u7528 Mock \u6570\u636e\u628a UI "
                            "\u9aa8\u67b6\u8dd1\u901a\uff0c\u540e\u9762"
                            "\u518d\u63a5 WebSocket\u3002"),
             true);
  AddMessage(QStringLiteral("\u6536\u5230\uff0c\u6d88\u606f\u6c14\u6ce1"
                            "\u548c\u8f93\u5165\u533a\u4e5f\u51c6\u5907"
                            "\u597d\u4e86\u3002"),
             false);
}

QWidget *ChatWindow::CreateTitleBar() {
  auto *title_bar = new QWidget(this);
  auto *layout = new QHBoxLayout(title_bar);
  auto *name_label = new QLabel(QStringLiteral("\u4e1c\u65b9-Askeai"), title_bar);
  auto *online_dot = new QLabel(title_bar);

  title_bar->setObjectName("chatTitleBar");
  name_label->setObjectName("chatTitleName");
  online_dot->setObjectName("onlineDot");
  online_dot->setFixedSize(8, 8);

  layout->setContentsMargins(20, 0, 16, 0);
  layout->setSpacing(8);
  layout->addWidget(name_label);
  layout->addWidget(online_dot);
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
  auto *send_button = new QPushButton(QStringLiteral("\u53d1\u9001"), send_row);
  auto *send_more_button = new QToolButton(send_row);

  input_area->setObjectName("inputArea");
  tool_bar->setObjectName("inputToolBar");
  send_button->setObjectName("sendButton");
  send_more_button->setObjectName("sendMoreButton");
  send_more_button->setText("v");

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
  send_layout->addWidget(send_button);
  send_layout->addWidget(send_more_button);

  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);
  root_layout->addWidget(tool_bar);
  root_layout->addWidget(message_input_, 1);
  root_layout->addWidget(send_row);

  connect(send_button, &QPushButton::clicked, this, &ChatWindow::HandleSend);
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

void ChatWindow::HandleSend() {
  const QString text = message_input_->toPlainText().trimmed();
  if (text.isEmpty()) {
    return;
  }

  AddMessage(text, true);
  message_input_->clear();
}

void ChatWindow::ScrollToBottom() {
  QTimer::singleShot(0, this, [this]() {
    message_scroll_area_->verticalScrollBar()->setValue(
        message_scroll_area_->verticalScrollBar()->maximum());
  });
}
