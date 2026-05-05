#pragma once

#include <QtCore/QHash>
#include <QtWidgets/QDialog>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

class CreateGroupDialog : public QDialog {
  Q_OBJECT

 public:
  explicit CreateGroupDialog(int default_user_id,
                             const QString &default_display_name,
                             QWidget *parent = nullptr);

 signals:
  void groupCreated(int group_id, const QString &group_name, int member_count);

 private:
  void LoadFriends();
  void AddFriend(int user_id, const QString &display_name);
  void UpdateCreateState();
  void CreateGroup();

  int default_user_id_;
  QLineEdit *name_edit_;
  QListWidget *member_list_;
  QLabel *hint_label_;
  QPushButton *create_button_;
};
