#pragma once

#include <QtCore/QString>
#include <QtWidgets/QMainWindow>

class ChatListWidget;
class ChatWindow;
class QCloseEvent;
class FriendListWidget;
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
  void OpenConversation(int target_user_id, const QString &display_name,
                        bool is_online);
  void RefreshFriendRequests();
  void RefreshPresence();
  void SwitchMiddlePanel(int index);

  bool session_initialized_ = false;
  TitleBar *title_bar_;
  NavBar *nav_bar_;
  ChatListWidget *chat_list_widget_;
  FriendListWidget *friend_list_widget_;
  ChatWindow *chat_window_;
  QStackedWidget *middle_stack_;
  QTimer *request_timer_;
  QTimer *presence_timer_;
};
