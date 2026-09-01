#include "VideoThread.h"

#include <opencv2/opencv.hpp>
#include <QDir>
#include <QDateTime>

VideoThread::VideoThread(QObject *parent) : QThread(parent) {}

VideoThread::~VideoThread()
{
    stop();
    wait();
}

void VideoThread::setSource(const QString &s, int id)
{
    source = s;
    cameraId = id;
}

void VideoThread::stop()
{
    running = false;
}

// v2: UI 线程调用，写入 atomic，采集线程每帧读取，无需加锁
void VideoThread::setSensitivity(double value)
{
    sensitivity = value;
}

// v2: 打开录像文件，开始录制（文件名带路号，避免多路同时录制互相覆盖）
void VideoThread::startRecording(const cv::Size &frameSize, double fps)
{
    // 自动创建录像目录（相对运行目录的 recordings/）
    QDir().mkpath(recordDir);

    QString name = recordDir + QString("/cam%1_").arg(cameraId) +
                   QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".mp4";

    // mp4v 是 OpenCV 里兼容性最好的 MP4 编码
    writer.open(name.toStdString(),
                cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
                fps, frameSize);

    recording = writer.isOpened();
    if (recording)
        emit recordingState(true);
}

// v2: 关闭录像文件（必须 release，否则文件损坏）
void VideoThread::stopRecording()
{
    if (recording)
    {
        writer.release();
        recording = false;
        emit recordingState(false);
    }
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

    // v2: 帧率统计——每秒发一次 fpsStat 信号
    int frameCount = 0;
    auto fpsWindowStart = std::chrono::steady_clock::now();

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

        // v2: 每帧应用 UI 设置的最新灵敏度
        detector.setSensitivity(sensitivity.load());

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

        // ===== v2: 运动自动录像 =====
        auto now = std::chrono::steady_clock::now();
        if (motion)
            lastMotionTime = now;

        if (motion && !recording)
            startRecording(frame.size(), (fps > 1.0) ? fps : 30.0);

        if (recording && !motion &&
            std::chrono::duration_cast<std::chrono::seconds>(now - lastMotionTime).count() >= stopDelaySec)
            stopRecording();

        if (recording)
            writer.write(processed); // 录画框后的帧，回放时能看到检测结果

        emit frameReady(matToQImage(processed));

        // v2: 帧率统计（按实际处理耗时计算，包含算法开销，比视频标称 fps 更真实）
        ++frameCount;
        double elapsed = std::chrono::duration<double>(now - fpsWindowStart).count();
        if (elapsed >= 1.0)
        {
            emit fpsStat(frameCount / elapsed);
            frameCount = 0;
            fpsWindowStart = now;
        }

        msleep(frameDelay);
    }

    // 线程退出前必须关闭录像文件
    stopRecording();
    cap.release();
    emit finished();
}
