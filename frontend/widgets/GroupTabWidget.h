#pragma once

#include <QtCore/QJsonArray>
#include <QtWidgets/QWidget>

class QVBoxLayout;

class GroupTabWidget : public QWidget {
  Q_OBJECT

 public:
  explicit GroupTabWidget(QWidget *parent = nullptr);
  void loadGroups();

 signals:
  void groupOpenRequested(int group_id, const QString &group_name,
                          int member_count, int my_role);
  void groupsChanged();

 private:
  void RenderGroups(const QJsonObject &data);
  QWidget *CreateGroupSection(const QString &title, const QJsonArray &groups,
                              int role);
  QWidget *CreateGroupItem(const QJsonObject &group, int role);
  void ShowGroupMenu(const QJsonObject &group, int role, QWidget *anchor);
  void LeaveGroup(int group_id);
  void DeleteGroup(int group_id);

  QVBoxLayout *content_layout_;
};
