#pragma once

#include <QtWidgets/QMainWindow>

class QToolButton;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);

 private:
  QWidget *CreateTopBar();
  void ShowUserMenu();
  void ShowProfileEditor();
  void LoadStyleSheet();

  QToolButton *avatar_button_;
};
