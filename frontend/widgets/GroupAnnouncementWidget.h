#pragma once

#include <QtWidgets/QWidget>

class QLabel;
class QToolButton;

class GroupAnnouncementWidget : public QWidget {
  Q_OBJECT

 public:
  explicit GroupAnnouncementWidget(QWidget *parent = nullptr);
  void setGroup(int group_id, bool can_edit);
  void loadAnnouncement();
  void editAnnouncement();

 signals:
  void announcementChanged();

 private:
  int group_id_ = 0;
  bool can_edit_ = false;
  QLabel *content_label_;
  QToolButton *edit_button_;
};
