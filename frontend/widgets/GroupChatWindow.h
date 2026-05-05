#pragma once

#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtWidgets/QWidget>

#include "../src/MessageBubble.h"

class ChatInputEdit;
class GroupAnnouncementWidget;
class GroupMemberWidget;
class GroupMessageBubble;
class QLabel;
class QPushButton;
class QScrollArea;
class QTimer;
class QToolButton;
class QVBoxLayout;

class GroupChatWindow : public QWidget {
  Q_OBJECT

 public:
  explicit GroupChatWindow(QWidget *parent = nullptr);
  void openGroup(int group_id, const QString &group_name, int member_count,
                 int my_role);

 signals:
  void groupMessageSent(int group_id, const QString &content,
                        const QString &created_at);
  void groupDeleted(int group_id);

 protected:
  void resizeEvent(QResizeEvent *event) override;

 private:
  QWidget *CreateTitleBar();
  QWidget *CreateMessageArea();
  QWidget *CreateInputArea();
  QWidget *CreateSidePanel();
  void LoadProfile();
  void LoadGroupInfo();
  void LoadMessages(bool full_refresh);
  void AppendMessage(const QJsonObject &message);
  void AddTimestamp(const QString &time_text);
  void RemoveAllMessages();
  void SendText();
  void ShowMoreMenu();
  void RenameGroup();
  void DissolveGroup();
  void ScrollToBottom();
  QString FormatTime(const QString &raw_time) const;
  int ResolveMessageType(const QJsonObject &message) const;
  int MaxBubbleWidth() const;
  void UpdateBubbleWidths();
  void OpenImage(int file_id, const QString &file_name, qint64 file_size);
  void DownloadFile(int file_id, const QString &file_name, qint64 file_size);

  int group_id_ = 0;
  int member_count_ = 0;
  int my_role_ = 0;
  int current_user_id_ = 0;
  int last_message_id_ = 0;
  QString group_name_;
  QString last_rendered_minute_;
  QSet<int> rendered_message_ids_;
  QLabel *title_label_;
  QScrollArea *message_scroll_area_;
  QVBoxLayout *message_layout_;
  ChatInputEdit *message_input_;
  QPushButton *send_button_;
  QToolButton *more_button_;
  GroupAnnouncementWidget *announcement_widget_;
  GroupMemberWidget *member_widget_;
  QTimer *message_timer_;
  QList<GroupMessageBubble *> bubbles_;
  bool message_request_pending_ = false;
};
