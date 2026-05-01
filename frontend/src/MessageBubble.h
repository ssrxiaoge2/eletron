#pragma once

#include <QtWidgets/QWidget>

class MessageBubble : public QWidget {
  Q_OBJECT

 public:
  MessageBubble(const QString &text, bool sent_by_me, QWidget *parent = nullptr);
};
