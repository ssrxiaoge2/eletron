#include "MainWindow.h"

#include "ChatListWidget.h"
#include "ChatWindow.h"
#include "NavBar.h"

#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

namespace {

QString TextBrandName() {
  return QStringLiteral("\u98de\u9e3d\u901a\u8baf");
}

QString TextNickname() {
  return QStringLiteral("\u6d4b\u8bd5\u7528\u6237");
}

QString TextSignature() {
  return QStringLiteral("\u4eca\u5929\u4e5f\u8981\u597d\u597d\u804a\u5929");
}

QString TextEditProfile() {
  return QStringLiteral("\u7f16\u8f91\u8d44\u6599");
}

}  // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle(TextBrandName());
  resize(1180, 760);
  LoadStyleSheet();

  auto *central_widget = new QWidget(this);
  auto *root_layout = new QVBoxLayout(central_widget);
  auto *splitter = new QSplitter(Qt::Horizontal, central_widget);

  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);

  splitter->setObjectName("mainSplitter");
  splitter->addWidget(new NavBar(splitter));
  splitter->addWidget(new ChatListWidget(splitter));
  splitter->addWidget(new ChatWindow(splitter));
  splitter->setSizes({60, 300, 820});
  splitter->setCollapsible(0, false);
  splitter->setCollapsible(1, false);
  splitter->setCollapsible(2, false);

  root_layout->addWidget(CreateTopBar());
  root_layout->addWidget(splitter, 1);

  setCentralWidget(central_widget);
}

QWidget *MainWindow::CreateTopBar() {
  auto *top_bar = new QWidget(this);
  auto *layout = new QHBoxLayout(top_bar);
  auto *brand_label = new QLabel(TextBrandName(), top_bar);
  auto *user_text_layout = new QVBoxLayout();
  auto *nickname_label = new QLabel(TextNickname(), top_bar);
  auto *signature_label = new QLabel(TextSignature(), top_bar);
  auto *minimize_button = new QToolButton(top_bar);
  auto *maximize_button = new QToolButton(top_bar);
  auto *close_button = new QToolButton(top_bar);

  top_bar->setObjectName("topBar");
  brand_label->setObjectName("topBrandLabel");
  nickname_label->setObjectName("topNicknameLabel");
  signature_label->setObjectName("topSignatureLabel");

  avatar_button_ = new QToolButton(top_bar);
  avatar_button_->setObjectName("topAvatarButton");
  avatar_button_->setText(QStringLiteral("\u9e3d"));
  avatar_button_->setPopupMode(QToolButton::InstantPopup);
  avatar_button_->setContextMenuPolicy(Qt::CustomContextMenu);

  user_text_layout->setContentsMargins(0, 0, 0, 0);
  user_text_layout->setSpacing(2);
  user_text_layout->addWidget(nickname_label);
  user_text_layout->addWidget(signature_label);

  minimize_button->setObjectName("windowButton");
  maximize_button->setObjectName("windowButton");
  close_button->setObjectName("closeButton");
  minimize_button->setText("-");
  maximize_button->setText("□");
  close_button->setText("x");

  layout->setContentsMargins(16, 0, 10, 0);
  layout->setSpacing(12);
  layout->addWidget(brand_label);
  layout->addSpacing(12);
  layout->addWidget(avatar_button_);
  layout->addLayout(user_text_layout);
  layout->addStretch();
  layout->addWidget(minimize_button);
  layout->addWidget(maximize_button);
  layout->addWidget(close_button);

  connect(avatar_button_, &QToolButton::clicked, this, &MainWindow::ShowUserMenu);
  connect(avatar_button_, &QWidget::customContextMenuRequested, this,
          [this](const QPoint &) { ShowUserMenu(); });
  connect(minimize_button, &QToolButton::clicked, this, &QWidget::showMinimized);
  connect(maximize_button, &QToolButton::clicked, this, [this]() {
    isMaximized() ? showNormal() : showMaximized();
  });
  connect(close_button, &QToolButton::clicked, this, &QWidget::close);

  return top_bar;
}

void MainWindow::ShowUserMenu() {
  QMenu menu(this);
  menu.addAction(TextNickname())->setEnabled(false);
  menu.addAction(TextSignature())->setEnabled(false);
  menu.addSeparator();
  QAction *edit_action = menu.addAction(TextEditProfile());
  connect(edit_action, &QAction::triggered, this, &MainWindow::ShowProfileEditor);
  menu.exec(avatar_button_->mapToGlobal(avatar_button_->rect().bottomLeft()));
}

void MainWindow::ShowProfileEditor() {
  QDialog dialog(this);
  auto *layout = new QVBoxLayout(&dialog);
  auto *nickname_edit = new QLineEdit(TextNickname(), &dialog);
  auto *signature_edit = new QTextEdit(TextSignature(), &dialog);
  auto *save_button =
      new QPushButton(QStringLiteral("\u4fdd\u5b58"), &dialog);

  dialog.setWindowTitle(TextEditProfile());
  nickname_edit->setPlaceholderText(QStringLiteral("\u6635\u79f0"));
  signature_edit->setPlaceholderText(QStringLiteral("\u4e2a\u6027\u7b7e\u540d"));
  layout->addWidget(nickname_edit);
  layout->addWidget(signature_edit);
  layout->addWidget(save_button);
  connect(save_button, &QPushButton::clicked, &dialog, &QDialog::accept);
  dialog.exec();
}

void MainWindow::LoadStyleSheet() {
  QFile style_file(":/MainWindow/styles/main.qss");
  if (style_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    setStyleSheet(QString::fromUtf8(style_file.readAll()));
  }
}
