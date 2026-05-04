#pragma once

#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QSet>
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
  void clearUnreadForConversation(int target_user_id);

signals:
  void conversationSelected(int target_user_id, const QString &target_username,
                            bool is_online);
  void onlineStatusChanged(int target_user_id, bool is_online);
  void newConversationRequested();

 private:
  void FetchConversations();
  void AddConversation(int target_user_id, const QString &target_username,
                       const QString &last_message,
                       const QString &last_message_time, int unread_count,
                       bool is_online);
  QString DisplayNameForConversation(const QJsonObject &item) const;
  void ClearUnreadBadge(QListWidgetItem *item);
  void MarkConversationRead(int target_user_id);
  bool OnlineStateForConversation(int target_user_id,
                                  const QJsonObject &item) const;
  void ApplyOnlineStatus(int target_user_id, bool is_online);
  QString FormatTime(const QString &raw_time) const;

  QListWidget *session_list_;
  QHash<int, QString> friend_nickname_cache_;
  QHash<int, bool> friend_online_cache_;
  QSet<int> read_conversation_ids_;
  QSet<int> read_request_pending_ids_;
};
