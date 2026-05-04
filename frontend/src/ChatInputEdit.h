#pragma once

#include <QtWidgets/QTextEdit>

class QDragEnterEvent;
class QDropEvent;
class QKeyEvent;
class QPixmap;

class ChatInputEdit : public QTextEdit {
  Q_OBJECT

 public:
  explicit ChatInputEdit(QWidget *parent = nullptr);

 signals:
  void imageDropped(const QString &file_path);
  void imagePasted(const QPixmap &pixmap);

 protected:
  void keyPressEvent(QKeyEvent *event) override;
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;
};
