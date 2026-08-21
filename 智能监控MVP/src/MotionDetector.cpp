#include "MotionDetector.h"

MotionDetector::MotionDetector()
    : bg(cv::createBackgroundSubtractorMOG2(500, 16, false))
{
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
        if (cv::contourArea(c) < 500)
            continue; // 过滤小噪点
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