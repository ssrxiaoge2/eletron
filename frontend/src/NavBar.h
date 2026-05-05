#pragma once

#include <QtWidgets/QWidget>

class QToolButton;

class NavBar : public QWidget {
  Q_OBJECT

 public:
  explicit NavBar(QWidget *parent = nullptr);
  void setCurrentIndex(int index);
  void setFriendBadge(int count);

signals:
  void currentIndexChanged(int index);
  void logoutRequested();

 private:
  QToolButton *CreateNavButton(const QString &text, bool checked, int index);

  QToolButton *message_button_;
  QToolButton *friend_button_;
  QToolButton *contact_button_;
};
