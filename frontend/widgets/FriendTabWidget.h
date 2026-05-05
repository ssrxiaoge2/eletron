#pragma once

#include <QtCore/QJsonArray>
#include <QtWidgets/QWidget>

class QVBoxLayout;

class FriendTabWidget : public QWidget {
  Q_OBJECT

 public:
  explicit FriendTabWidget(QWidget *parent = nullptr);
  void loadGroups();

 signals:
  void friendMessageRequested(int user_id, const QString &display_name,
                              bool is_online);

 private:
  void RenderGroups(const QJsonArray &groups);
  QWidget *CreateGroupBlock(const QJsonObject &group, const QJsonArray &groups);
  QWidget *CreateFriendItem(const QJsonObject &friend_item,
                            const QJsonArray &groups);
  void ShowGroupMenu(int group_id, const QString &group_name,
                     QWidget *anchor);
  void ShowFriendMenu(const QJsonObject &friend_item, const QJsonArray &groups,
                      QWidget *anchor);
  void AddGroup();
  void RenameGroup(int group_id, const QString &old_name);
  void DeleteGroup(int group_id);
  void MoveFriendToGroup(int friend_user_id, int group_id);

  QVBoxLayout *content_layout_;
};
