#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QVector>
#include <QString>

class QLabel;
class QLineEdit;
class QPushButton;
class QGridLayout;
class QCheckBox;
class VideoThread;
class StreamServer;

// v4: 一路视频源对应一个格子（线程 + 画面 + 标题 + 计数）
struct CameraCell
{
    VideoThread *thread = nullptr;
    QLabel *videoLabel = nullptr;
    QLabel *nameLabel = nullptr;
    QLabel *countLabel = nullptr;  // v4: 人物计数标签
};

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddCamera();
    void onStopAll();
    void onToggleCounting(bool checked);

private:
    void addCell(const QString &source);
    void startStreamServer();       // v5: 启动局域网流媒体服务
    QString localIpAddress() const; // v5: 获取本机局域网IP

    QLineEdit *sourceEdit;
    QPushButton *addBtn;
    QPushButton *stopAllBtn;
    QCheckBox *countingCheckBox;  // v4: 计数功能开关
    QLabel *infoLabel;
    QGridLayout *gridLayout;

    StreamServer *streamServer = nullptr;  // v5: 局域网推流

    QVector<CameraCell> cells;      // 已添加的视频路
    int cameraIdCounter = 1;        // 路号递增，用于录像文件命名
    static const int MAX_CELLS = 9; // 九宫格上限
};

#endif
