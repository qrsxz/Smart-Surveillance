#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class VideoThread;

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStart();
    void onStop();
    void onFrame(const QImage &image);
    void onMotion(bool detected);

private:
    QLabel *videoLabel;
    QLabel *statusLabel;
    QLineEdit *sourceEdit;
    QPushButton *startBtn;
    QPushButton *stopBtn;
    VideoThread *thread;
};

#endif
