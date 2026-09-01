#include "MotionDetector.h"

#include <algorithm>

MotionDetector::MotionDetector()
    : bg(cv::createBackgroundSubtractorMOG2(500, 16, false))
{
    setSensitivity(0.5); // 默认中等灵敏度
}

// v2: 灵敏度映射——
//   灵敏度 0.0（最不灵敏）：面积阈值 2000 像素，只报大目标
//   灵敏度 1.0（最灵敏）  ：面积阈值 100 像素，小目标也报
// 同时同步调整 MOG2 的方差阈值（16~48，越灵敏越低）
void MotionDetector::setSensitivity(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    minContourArea = 2000.0 - value * 1900.0;              // 2000 -> 100
    bg->setVarThreshold(48.0 - value * 32.0);              // 48   -> 16
}

cv::Mat MotionDetector::process(const cv::Mat &frame)
{
    // 1. 背景减除得到前景掩码
    cv::Mat fgMask;
    bg->apply(frame, fgMask);

    // 2. 形态学开/闭运算去噪
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(fgMask, fgMask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(fgMask, fgMask, cv::MORPH_CLOSE, kernel);

    // 3. 提取轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(fgMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // 4. 过滤小噪点，所有运动区域合并为一个大框
    motionDetected = false;
    cv::Rect merged;
    for (const auto &c : contours)
    {
        if (cv::contourArea(c) < minContourArea) // v2: 阈值由灵敏度决定
            continue;
        cv::Rect r = cv::boundingRect(c);
        merged = (merged.width == 0) ? r : (merged | r);
        motionDetected = true;
    }

    // 5. 无运动直接返回原帧（浅拷贝，开销极小）；有运动才拷贝画框
    if (!motionDetected)
        return frame;

    cv::Mat output;
    frame.copyTo(output);
    cv::rectangle(output, merged, cv::Scalar(0, 255, 0), 2);
    return output;
}
