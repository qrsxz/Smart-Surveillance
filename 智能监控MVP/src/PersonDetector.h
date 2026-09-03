#ifndef PERSONDETECTOR_H
#define PERSONDETECTOR_H

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <vector>

// 人物检测结果
struct PersonDetection
{
    cv::Rect bbox;      // 边界框
    double confidence;  // 置信度
};

// 人物检测器：使用HOG + SVM行人检测
class PersonDetector
{
public:
    PersonDetector();

    // 处理一帧，返回画好检测框的帧
    cv::Mat process(const cv::Mat &frame);

    // 获取检测到的人物数量
    int getPersonCount() const { return lastCount; }

    // 是否检测到人物
    bool hasPerson() const { return lastCount > 0; }

    // 设置是否启用计数显示
    void setCountingEnabled(bool enabled) { countingEnabled = enabled; }
    bool isCountingEnabled() const { return countingEnabled; }

private:
    cv::HOGDescriptor hog;
    int lastCount = 0;
    bool countingEnabled = true;  // 计数功能开关

    // 检测结果过滤
    std::vector<PersonDetection> detect(const cv::Mat &frame);
    void drawDetections(cv::Mat &frame, const std::vector<PersonDetection> &detections);
};

#endif // PERSONDETECTOR_H
