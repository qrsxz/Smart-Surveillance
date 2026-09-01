#ifndef MOTIONDETECTOR_H
#define MOTIONDETECTOR_H

#include <opencv2/opencv.hpp>

// 运动检测：MOG2 背景减除 + 形态学去噪 + 轮廓画框
// v2：新增 setSensitivity()，供 UI 灵敏度滑块调节（成员A的 UI 依赖此接口）
// 注意：本类属于成员B的 algorithm 层，当前为 v1 基础上加接口的过渡版本，
//       v2 联调时以成员B的调优版本为准，但请保留 setSensitivity 接口签名
class MotionDetector
{
public:
    MotionDetector();

    // 处理一帧，返回画好运动框的帧；用 hasMotion() 查询本帧是否有运动
    cv::Mat process(const cv::Mat &frame);
    bool hasMotion() const { return motionDetected; }

    // v2: 灵敏度 0.0~1.0，越大越灵敏（影响轮廓面积阈值与 MOG2 方差阈值）
    void setSensitivity(double value);

private:
    cv::Ptr<cv::BackgroundSubtractorMOG2> bg;
    bool motionDetected = false;

    // v2: 由灵敏度映射出的参数（在 setSensitivity 中计算）
    double minContourArea = 500.0;  // 轮廓面积阈值：灵敏度越高阈值越低
};

#endif // MOTIONDETECTOR_H
