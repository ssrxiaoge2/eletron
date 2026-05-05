#pragma once

#include <QtWidgets/QWidget>

#include "../src/MessageBubble.h"

class GroupMessageBubble : public QWidget {
  Q_OBJECT

 public:
  GroupMessageBubble(const QString &content, MessageBubble::BubbleType type,
                     bool is_self, const QString &sender_nickname,
                     int sender_role, const QString &time,
                     QWidget *parent = nullptr);
  void setMaxBubbleWidth(int max_width);

 signals:
  void imageOpenRequested(int file_id, const QString &file_name,
                          qint64 file_size);
  void fileDownloadRequested(int file_id, const QString &file_name,
                             qint64 file_size);

 private:
  MessageBubble *bubble_;
};
