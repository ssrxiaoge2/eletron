#pragma once

#include <QtCore/QString>
#include <QtWidgets/QWidget>

class QLabel;
class QEvent;

class MessageBubble : public QWidget {
  Q_OBJECT

 public:
  enum BubbleType {
    Text = 0,
    Image = 1,
    File = 2,
  };

  MessageBubble(const QString &content, BubbleType type, bool is_self,
                const QString &time, QWidget *parent = nullptr);
  void setMaxBubbleWidth(int max_width);

 signals:
  void imageOpenRequested(int file_id, const QString &file_name,
                          qint64 file_size);
  void fileDownloadRequested(int file_id, const QString &file_name,
                             qint64 file_size);

 protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

 private:
  QWidget *CreateContentWidget(const QString &content);
  QWidget *CreateTextContent(const QString &content);
  QWidget *CreateImageContent(const QString &content);
  QWidget *CreateFileContent(const QString &content);
  void LoadThumbnail();

  QWidget *bubble_ = nullptr;
  QLabel *thumbnail_label_ = nullptr;
  QWidget *clickable_widget_ = nullptr;
  BubbleType type_ = Text;
  bool is_self_ = false;
  int file_id_ = 0;
  QString file_name_;
  qint64 file_size_ = 0;
};
