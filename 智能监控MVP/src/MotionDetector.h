#ifndef MOTIONDETECTOR_H
#define MOTIONDETECTOR_H

#include <opencv2/opencv.hpp>

// 运动检测：MOG2 背景减除 + 形态学去噪 + 轮廓画框
class MotionDetector
{
public:
    MotionDetector();

    // 处理一帧，返回画好运动框的帧；用 hasMotion() 查询本帧是否有运动
    cv::Mat process(const cv::Mat &frame);
    bool hasMotion() const { return motionDetected; }

private:
    cv::Ptr<cv::BackgroundSubtractorMOG2> bg;
    bool motionDetected = false;
};

#endif
