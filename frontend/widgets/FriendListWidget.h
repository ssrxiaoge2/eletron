#pragma once

#include <QtWidgets/QWidget>

class QListWidget;
class QJsonArray;
class QVBoxLayout;
class QWidget;

class FriendListWidget : public QWidget {
  Q_OBJECT

 public:
  explicit FriendListWidget(QWidget *parent = nullptr);
  void loadFriends();
  void loadRequests();

 signals:
  void friendActivated(int target_user_id, const QString &display_name,
                       bool is_online);
  void requestCountChanged(int count);

 private:
  void AddFriend(int user_id, const QString &username, const QString &nickname,
                 const QString &signature, bool is_online);
  void RenderRequests(const QJsonArray &requests);
  void HandleRequest(int request_id, int action);

  QWidget *request_panel_;
  QVBoxLayout *request_layout_;
  QListWidget *friend_list_;
};
