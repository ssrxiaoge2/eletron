#pragma once

#include <QtCore/QPoint>
#include <QtWidgets/QWidget>

class QMouseEvent;
class QToolButton;

class TitleBar : public QWidget {
  Q_OBJECT

 public:
  explicit TitleBar(QWidget *parent = nullptr);

 protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;

 private:
  void ToggleMaximized();
  void UpdateMaximizeButton();

  bool dragging_ = false;
  QPoint drag_pos_;
  QToolButton *maximize_button_;
};
