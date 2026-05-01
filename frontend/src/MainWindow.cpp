#include "MainWindow.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("QQ Desktop");
  resize(960, 640);

  auto *central_widget = new QWidget(this);
  auto *layout = new QVBoxLayout(central_widget);
  auto *placeholder = new QLabel("Main window skeleton - TODO", central_widget);

  placeholder->setAlignment(Qt::AlignCenter);
  layout->addWidget(placeholder);

  setCentralWidget(central_widget);
}
