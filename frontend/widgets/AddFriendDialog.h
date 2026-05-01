#pragma once

#include <QtWidgets/QDialog>

class QLineEdit;
class QListWidget;
class QPushButton;

class AddFriendDialog : public QDialog {
  Q_OBJECT

 public:
  explicit AddFriendDialog(QWidget *parent = nullptr);

 signals:
  void friendsChanged();

 private:
  void SearchUsers();
  void AddResult(int user_id, const QString &username, const QString &nickname,
                 const QString &signature, int friend_status);
  void ApplyFriend(int user_id, QPushButton *button);
  void AcceptFriend(int user_id, QPushButton *button);

  QLineEdit *search_edit_;
  QListWidget *result_list_;
};
