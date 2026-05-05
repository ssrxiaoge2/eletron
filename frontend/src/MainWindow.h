#pragma once

#include <QtCore/QString>
#include <QtWidgets/QMainWindow>

class ChatListWidget;
class ChatWindow;
class QCloseEvent;
class FriendListWidget;
class FriendManagerWidget;
class GroupChatWindow;
class NavBar;
class QStackedWidget;
class QTimer;
class TitleBar;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);
  void initializeSession();

 signals:
  void accountLoggedOut(const QString &username);

 protected:
  void closeEvent(QCloseEvent *event) override;

 private:
  void LoadStyleSheet();
  void LoadProfile();
  void OfflineOnClose();
  void LogoutAccount();
  void OpenAddFriendDialog();
  void OpenCreateGroupDialog(int default_user_id,
                             const QString &default_display_name);
  void OpenConversation(int target_user_id, const QString &display_name,
                        bool is_online);
  void OpenGroupConversation(int group_id, const QString &group_name,
                             int member_count, int my_role);
  void HandleGroupRemoved(int group_id);
  void RefreshFriendRequests();
  void RefreshPresence();
  void SwitchMiddlePanel(int index);

  bool session_initialized_ = false;
  TitleBar *title_bar_;
  NavBar *nav_bar_;
  ChatListWidget *chat_list_widget_;
  FriendListWidget *friend_list_widget_;
  FriendManagerWidget *friend_manager_widget_;
  ChatWindow *chat_window_;
  GroupChatWindow *group_chat_window_;
  QStackedWidget *middle_stack_;
  QStackedWidget *chat_stack_;
  QTimer *request_timer_;
  QTimer *presence_timer_;
};
