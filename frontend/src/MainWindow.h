#pragma once

#include <QtWidgets/QMainWindow>

class ChatListWidget;
class ChatWindow;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);

 private:
  void LoadStyleSheet();

  ChatListWidget *chat_list_widget_;
  ChatWindow *chat_window_;
};
