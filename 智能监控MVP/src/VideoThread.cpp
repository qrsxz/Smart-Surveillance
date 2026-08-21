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
    cv::VideoCapture cap;
    bool ok = false;

    if (source.isEmpty() || source == "0")
        ok = cap.open(0); // 默认摄像头
    else
        ok = cap.open(source.toStdString()); // 本地视频文件

    if (!ok)
    {
        emit finishedWithError(QString("无法打开视频源: %1").arg(source));
        return;
    }

    running = true;
    cv::Mat frame;

    while (running)
    {
        if (!cap.read(frame) || frame.empty())
        {
            // 本地视频文件播完后循环播放
            if (source != "0")
            {
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                continue;
            }
            break;
        }

        cv::Mat processed = detector.process(frame);
        emit motionState(detector.hasMotion());
        emit frameReady(matToQImage(processed));

        msleep(30); // 控制帧率，降低 CPU 占用
    }

    cap.release();
}
