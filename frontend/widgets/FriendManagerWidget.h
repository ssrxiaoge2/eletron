#pragma once

#include <QtWidgets/QWidget>

class FriendTabWidget;
class GroupTabWidget;
class QJsonArray;
class QVBoxLayout;

class FriendManagerWidget : public QWidget {
  Q_OBJECT

 public:
  explicit FriendManagerWidget(QWidget *parent = nullptr);
  void loadData();
  void loadNotifications();

 signals:
  void friendMessageRequested(int user_id, const QString &display_name,
                              bool is_online);
  void groupOpenRequested(int group_id, const QString &group_name,
                          int member_count, int my_role);
  void dataChanged();

 private:
  QWidget *CreateNotificationRow(const QString &title);
  void RenderFriendRequests(const QJsonArray &requests);
  void HandleFriendRequest(int request_id, int action);

  FriendTabWidget *friend_tab_;
  GroupTabWidget *group_tab_;
  QWidget *request_panel_;
  QVBoxLayout *request_layout_;
};
