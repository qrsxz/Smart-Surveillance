#ifndef VIDEOTHREAD_H
#define VIDEOTHREAD_H

#include <QThread>
#include <QImage>
#include <QString>
#include <atomic>
#include <chrono>

#include <opencv2/videoio.hpp>

#include "MotionDetector.h"

// 采集线程：读帧 -> 运动检测 -> (运动时自动录像) -> 发信号给 UI，不阻塞界面
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
    void recordingState(bool recording);        // v2: 录像状态变化
    void finished();                            // 视频源正常结束（文件播完/摄像头断开）
    void finishedWithError(const QString &msg);

protected:
    void run() override;

private:
    void startRecording(const cv::Size &frameSize, double fps);
    void stopRecording();

    QString source;
    std::atomic<bool> running{false};
    MotionDetector detector;

    // 运动防抖计数（连续帧），避免状态栏狂闪
    int motionStreak = 0;
    int noMotionStreak = 0;

    // v2: 运动自动录像
    cv::VideoWriter writer;
    bool recording = false;
    std::chrono::steady_clock::time_point lastMotionTime;
    const int stopDelaySec = 3;                 // 静止超过 3 秒停止录像
    QString recordDir = "recordings";           // 录像输出目录（相对运行目录）
};

#endif