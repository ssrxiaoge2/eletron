#pragma once

#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QSet>
#include <QtCore/QVector>
#include <QtWidgets/QWidget>

class QListWidget;
class QListWidgetItem;

class ChatListWidget : public QWidget {
  Q_OBJECT

 public:
  explicit ChatListWidget(QWidget *parent = nullptr);
  void loadConversations();
  void refreshConversations();
  void refreshOnlineStatuses();
  bool activateConversation(int target_user_id);
  bool hasConversation(int target_user_id) const;
  void updateConversationPreview(int target_user_id, const QString &content,
                                 const QString &created_at);
  void updateGroupPreview(int group_id, const QString &content,
                          const QString &created_at);
  void clearUnreadForConversation(int target_user_id);
  void refreshGroups();

signals:
  void conversationSelected(int target_user_id, const QString &target_username,
                            bool is_online);
  void groupConversationSelected(int group_id, const QString &group_name,
                                 int member_count, int my_role);
  void groupConversationRemoved(int group_id);
  void onlineStatusChanged(int target_user_id, bool is_online);
  void newConversationRequested();

 private:
  void FetchConversations();
  void FetchGroups(QVector<QJsonObject> private_conversations);
  void RenderConversations(const QVector<QJsonObject> &items);
  void AddConversation(int target_user_id, const QString &target_username,
                       const QString &last_message,
                       int last_message_type,
                       const QString &last_message_time, int unread_count,
                       bool is_online);
  void AddGroupConversation(int group_id, const QString &group_name,
                            int member_count, int my_role,
                            const QString &last_message,
                            int last_message_type,
                            const QString &last_message_time,
                            int unread_count);
  QString DisplayNameForConversation(const QJsonObject &item) const;
  void ShowContextMenu(const QPoint &pos);
  void ClearUnreadBadge(QListWidgetItem *item);
  void MarkConversationRead(int target_user_id);
  bool OnlineStateForConversation(int target_user_id,
                                  const QJsonObject &item) const;
  void ApplyOnlineStatus(int target_user_id, bool is_online);
  QString FormatTime(const QString &raw_time) const;
  QString FormatPreview(const QString &content, int type) const;

  QListWidget *session_list_;
  QHash<int, QString> friend_nickname_cache_;
  QHash<int, bool> friend_online_cache_;
  QSet<int> read_conversation_ids_;
  QSet<int> read_request_pending_ids_;
};
