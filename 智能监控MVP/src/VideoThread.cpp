#include "VideoThread.h"

#include <opencv2/opencv.hpp>

VideoThread::VideoThread(QObject *parent) : QThread(parent) {}

VideoThread::~VideoThread()
{
    stop();
    wait();
}

void VideoThread::setSource(const QString &s)
{
    source = s;
}

void VideoThread::stop()
{
    running = false;
}

// cv::Mat -> QImage（注意 BGR->RGB + 深拷贝）
static QImage matToQImage(const cv::Mat &mat)
{
    if (mat.type() == CV_8UC3)
    {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
    }
    else if (mat.type() == CV_8UC1)
    {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
    }
    return QImage();
}

void VideoThread::run()
{
    // 本地拷贝 source，避免与主线程的数据竞争
    const QString src = source;
    const bool isCamera = (src.isEmpty() || src == "0");

    cv::VideoCapture cap;
    const bool ok = isCamera ? cap.open(0) : cap.open(src.toStdString());

    if (!ok)
    {
        emit finishedWithError(QString("无法打开视频源: %1").arg(src));
        return;
    }

    // 按视频自身帧率决定延时，播放速度才正确；摄像头无稳定帧率则默认 30fps
    double fps = cap.get(cv::CAP_PROP_FPS);
    int frameDelay = (fps > 1.0) ? static_cast<int>(1000.0 / fps) : 33;

    running = true;
    cv::Mat frame;
    bool lastMotion = false;

    while (running)
    {
        if (!cap.read(frame) || frame.empty())
        {
            if (isCamera)
                break; // 摄像头断开，退出

            // 文件播完：尝试回到开头循环播放；失败则退出，避免死循环
            if (!cap.set(cv::CAP_PROP_POS_FRAMES, 0))
            {
                emit finishedWithError("视频文件播放结束且无法循环播放");
                break;
            }
            continue;
        }

        cv::Mat processed = detector.process(frame);

        // 运动防抖：连续 3 帧有运动才判定，连续 3 帧无运动才解除，状态变化时才发信号
        bool motion = detector.hasMotion();
        motionStreak = motion ? motionStreak + 1 : 0;
        noMotionStreak = motion ? 0 : noMotionStreak + 1;

        bool stableMotion = lastMotion;
        if (motion && motionStreak >= 3)
            stableMotion = true;
        if (!motion && noMotionStreak >= 3)
            stableMotion = false;

        if (stableMotion != lastMotion)
        {
            lastMotion = stableMotion;
            emit motionState(stableMotion);
        }

        emit frameReady(matToQImage(processed));
        msleep(frameDelay);
    }

    cap.release();
    emit finished(); // 通知 UI 复位按钮状态
}