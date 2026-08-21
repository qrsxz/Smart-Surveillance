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
// v3: 一个线程处理一路视频源，可被 MainWindow 实例化多个形成九宫格
class VideoThread : public QThread
{
    Q_OBJECT
public:
    explicit VideoThread(QObject *parent = nullptr);
    ~VideoThread();

    // source: "0" = 默认摄像头；否则为视频文件/RTSP 地址
    // id: 路号，用于录像文件名区分（cam1_xxx.mp4, cam2_xxx.mp4 ...）
    void setSource(const QString &source, int id);
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
    int cameraId = 0;                           // v3: 路号，用于录像文件命名
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