#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QImage>
#include <QString>

// v3 成员A：九宫格单元格视频控件
// 用 QPainter 绘制视频帧（保持宽高比居中），画面叠加：
//   - 左上角：摄像头名称（CAM 1、CAM 2 ...）
//   - 名称下方：REC 红点（录制中）
//   - 右上角：运动状态（检测到运动 / 正常）
//   - 边框颜色随状态变化：灰=空闲 绿=正常 黄=运动 红=录制
// 双击发出 zoomRequested()，由 MainWindow 切换到放大视图（Esc 返回）
class VideoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoWidget(QWidget *parent = nullptr);

    void setCameraName(const QString &name);   // 设置格子标题（叠加在画面左上角）
    QString cameraName() const { return camName; }

    QSize minimumSizeHint() const override { return {240, 180}; }

public slots:
    void updateFrame(const QImage &image);   // 接收 VideoThread 的帧（跨线程队列连接）
    void setMotionState(bool detected);      // 运动状态
    void setRecordingState(bool recording);  // 录制状态
    void clearFrame();                       // 断开连接时清空画面

signals:
    void zoomRequested();                    // 双击格子，请求放大/还原

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QImage frame;
    QString camName;
    bool motionDetected = false;
    bool recording = false;
};

#endif // VIDEOWIDGET_H
