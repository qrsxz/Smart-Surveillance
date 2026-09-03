#include "PersonDetector.h"

PersonDetector::PersonDetector()
{
    // 初始化HOG行人检测器
    hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());
}

std::vector<PersonDetection> PersonDetector::detect(const cv::Mat &frame)
{
    std::vector<PersonDetection> detections;
    std::vector<cv::Rect> found;
    std::vector<double> weights;

    // 多尺度检测
    hog.detectMultiScale(frame, found, weights, 0, cv::Size(8, 8), cv::Size(0, 0), 1.05, 2, false);

    // 合并重叠的检测框 - NMSBoxes需要float类型
    std::vector<float> weightsFloat(weights.begin(), weights.end());
    std::vector<int> indices;
    cv::dnn::NMSBoxes(found, weightsFloat, 0.5, 0.4, indices);

    for (int idx : indices)
    {
        PersonDetection det;
        det.bbox = found[idx];
        det.confidence = weights[idx];
        detections.push_back(det);
    }

    return detections;
}

void PersonDetector::drawDetections(cv::Mat &frame, const std::vector<PersonDetection> &detections)
{
    for (const auto &det : detections)
    {
        // 绘制边界框（绿色）
        cv::rectangle(frame, det.bbox, cv::Scalar(0, 255, 0), 2);

        // 绘制置信度标签
        std::string label = cv::format("Person: %.2f", det.confidence);
        int baseline = 0;
        cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
        cv::rectangle(frame, 
                     cv::Point(det.bbox.x, det.bbox.y - labelSize.height - 10),
                     cv::Point(det.bbox.x + labelSize.width, det.bbox.y),
                     cv::Scalar(0, 255, 0), -1);
        cv::putText(frame, label, 
                   cv::Point(det.bbox.x, det.bbox.y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    }

    // 如果启用了计数功能，显示人物数量
    if (countingEnabled && !detections.empty())
    {
        std::string countText = cv::format("Person Count: %d", (int)detections.size());
        cv::putText(frame, countText, cv::Point(10, 30), 
                   cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 255), 2);
    }
}

cv::Mat PersonDetector::process(const cv::Mat &frame)
{
    // 检测人物
    std::vector<PersonDetection> detections = detect(frame);
    lastCount = detections.size();

    // 如果没有检测到人物，直接返回原帧
    if (detections.empty())
    {
        return frame;
    }

    // 绘制检测结果
    cv::Mat output;
    frame.copyTo(output);
    drawDetections(output, detections);

    return output;
}
