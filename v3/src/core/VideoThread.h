#ifndef VIDEOTHREAD_H
#define VIDEOTHREAD_H

#include <QThread>
#include <QImage>
#include <QString>
#include <atomic>
#include <chrono>

#include <opencv2/videoio.hpp>

#include "../algorithm/MotionDetector.h"

// 采集线程：读帧 -> 运动检测 -> (运动时自动录像) -> 发信号给 UI，不阻塞界面
// v2：相对 v1 新增——
//   1. fpsStat 信号：每秒统计一次实际处理帧率，供 UI 实时显示（成员A需求）
//   2. setSensitivity()：接收 UI 灵敏度滑块的值，转发给 MotionDetector（成员A需求）
// 注意：本类属于成员C的 core 层，v2 联调前由成员A临时维护，联调时与成员C版本合并
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

    // v2: 设置运动检测灵敏度（0.0~1.0，越大越灵敏），UI 线程调用，线程安全
    void setSensitivity(double value);

signals:
    void frameReady(const QImage &image);
    void motionState(bool detected);
    void recordingState(bool recording);        // v2: 录像状态变化
    void fpsStat(double fps);                   // v2: 每秒一次的实际处理帧率
    void finished();                            // 视频源正常结束（文件播完/摄像头断开）
    void finishedWithError(const QString &msg);

protected:
    void run() override;

private:
    void startRecording(const cv::Size &frameSize, double fps);
    void stopRecording();

    QString source;
    int cameraId = 0;                           // 路号，用于录像文件命名
    std::atomic<bool> running{false};
    MotionDetector detector;

    // v2: 灵敏度（atomic 保证 UI 线程写入、采集线程读取的安全性）
    std::atomic<double> sensitivity{0.5};

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

#endif // VIDEOTHREAD_H
