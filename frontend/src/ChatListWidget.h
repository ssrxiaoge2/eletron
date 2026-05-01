#pragma once

#include <QtWidgets/QWidget>

class QListWidget;
class QListWidgetItem;

class ChatListWidget : public QWidget {
  Q_OBJECT

 public:
  explicit ChatListWidget(QWidget *parent = nullptr);
  void loadConversations();
  bool activateConversation(int target_user_id);
  bool hasConversation(int target_user_id) const;
  void updateConversationPreview(int target_user_id, const QString &content,
                                 const QString &created_at);

 signals:
  void conversationSelected(int target_user_id, const QString &target_username,
                            bool is_online);
  void newConversationRequested();

 private:
  void AddConversation(int target_user_id, const QString &target_username,
                       const QString &last_message,
                       const QString &last_message_time, int unread_count,
                       bool is_online);
  void ClearUnreadBadge(QListWidgetItem *item);
  QString FormatTime(const QString &raw_time) const;

  QListWidget *session_list_;
};
