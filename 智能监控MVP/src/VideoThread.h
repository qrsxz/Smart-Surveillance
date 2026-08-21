#ifndef VIDEOTHREAD_H
#define VIDEOTHREAD_H

#include <QThread>
#include <QImage>
#include <QString>
#include <atomic>

#include "MotionDetector.h"

// 采集线程：读帧 -> 运动检测 -> 发信号给 UI，不阻塞界面
class VideoThread : public QThread
{
    Q_OBJECT
public:
    explicit VideoThread(QObject *parent = nullptr);
    ~VideoThread();

    // source: "0" = 默认摄像头；否则为视频文件路径
    void setSource(const QString &source);
    void stop();

signals:
    void frameReady(const QImage &image);
    void motionState(bool detected);
    void finished();                            // 视频源正常结束（文件播完/摄像头断开）
    void finishedWithError(const QString &msg);

protected:
    void run() override;

private:
    QString source;
    std::atomic<bool> running{false};
    MotionDetector detector;

    // 运动防抖计数（连续帧），避免状态栏狂闪
    int motionStreak = 0;
    int noMotionStreak = 0;
};

#endif