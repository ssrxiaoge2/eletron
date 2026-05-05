#pragma once

#include <QtCore/QJsonObject>
#include <QtWidgets/QWidget>

class QLineEdit;
class QListWidget;

class GroupMemberWidget : public QWidget {
  Q_OBJECT

 public:
  explicit GroupMemberWidget(QWidget *parent = nullptr);
  void setGroup(int group_id, int my_role, int current_user_id);
  void loadMembers();

 signals:
  void membersChanged(int member_count);

 private:
  void AddMember(const QJsonObject &member);
  void ShowContextMenu(const QPoint &pos);
  void SetRole(int user_id, int role);
  void MuteMember(const QJsonObject &member);
  void KickMember(const QJsonObject &member);

  int group_id_ = 0;
  int my_role_ = 0;
  int current_user_id_ = 0;
  QLineEdit *search_edit_;
  QListWidget *member_list_;
};
