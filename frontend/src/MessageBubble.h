#pragma once

#include <QtWidgets/QWidget>

class QLabel;

class MessageBubble : public QWidget {
  Q_OBJECT

 public:
  MessageBubble(const QString &text, bool sent_by_me, QWidget *parent = nullptr);

 protected:
  void resizeEvent(QResizeEvent *event) override;

 private:
  QLabel *bubble_;
};
