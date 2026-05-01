#pragma once

#include <QtWidgets/QWidget>

class QListWidget;

class ChatListWidget : public QWidget {
  Q_OBJECT

 public:
  explicit ChatListWidget(QWidget *parent = nullptr);

 private:
  void AddMockSession(const QString &name, const QString &time,
                      const QString &preview, int unread_count);

  QListWidget *session_list_;
};
