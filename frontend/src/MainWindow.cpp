#include "MainWindow.h"

#include "ChatListWidget.h"
#include "ChatWindow.h"
#include "NavBar.h"
#include "TitleBar.h"

#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowFlags(Qt::FramelessWindowHint);
  setAttribute(Qt::WA_TranslucentBackground, false);
  setWindowTitle(QStringLiteral("\u98de\u9e3d\u901a\u8baf"));
  resize(1180, 760);
  LoadStyleSheet();

  auto *central_widget = new QWidget(this);
  auto *root_layout = new QVBoxLayout(central_widget);
  auto *splitter = new QSplitter(Qt::Horizontal, central_widget);

  chat_list_widget_ = new ChatListWidget(splitter);
  chat_window_ = new ChatWindow(splitter);

  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);

  splitter->setObjectName("mainSplitter");
  splitter->addWidget(new NavBar(splitter));
  splitter->addWidget(chat_list_widget_);
  splitter->addWidget(chat_window_);
  splitter->setSizes({60, 300, 820});
  splitter->setCollapsible(0, false);
  splitter->setCollapsible(1, false);
  splitter->setCollapsible(2, false);

  root_layout->addWidget(new TitleBar(central_widget));
  root_layout->addWidget(splitter, 1);

  setCentralWidget(central_widget);

  connect(chat_list_widget_, &ChatListWidget::conversationSelected,
          chat_window_, &ChatWindow::loadMessages);
  connect(chat_window_, &ChatWindow::messageSent, chat_list_widget_,
          &ChatListWidget::updateConversationPreview);

  chat_list_widget_->loadConversations();
}

void MainWindow::LoadStyleSheet() {
  QFile style_file(":/MainWindow/styles/main.qss");
  if (style_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    setStyleSheet(QString::fromUtf8(style_file.readAll()));
  }
}
