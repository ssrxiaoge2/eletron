#pragma once

#include <QtCore/QPoint>
#include <QtWidgets/QWidget>

class QMouseEvent;
class QLabel;
class QToolButton;

class TitleBar : public QWidget {
  Q_OBJECT

 public:
  explicit TitleBar(QWidget *parent = nullptr);
  void setUserInfo(const QString &username, const QString &nickname,
                   const QString &signature, const QString &avatar,
                   const QString &gender = QString(),
                   const QString &birthday = QString(),
                   const QString &created_at = QString(),
                   const QString &email = QString());

 signals:
  void profileUpdated(const QString &nickname, const QString &signature);

 protected:
  bool eventFilter(QObject *watched, QEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;

 private:
  void ToggleMaximized();
  void UpdateMaximizeButton();
  void ShowProfileDialog();

  bool dragging_ = false;
  QPoint drag_pos_;
  QString username_;
  QString nickname_;
  QString signature_;
  QString avatar_;
  QString gender_;
  QString birthday_;
  QString created_at_;
  QString email_;
  QLabel *avatar_label_;
  QLabel *info_label_;
  QToolButton *maximize_button_;
};
