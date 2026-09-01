#ifndef STATUSLED_H
#define STATUSLED_H

#include <QWidget>
#include <QColor>
#include <QTimer>

// v2 成员A：圆形状态指示灯控件（QPainter 绘制）
// 用途：录制状态指示灯（灰=待机 / 红色闪烁=录制中）、连接状态灯等
class StatusLed : public QWidget
{
    Q_OBJECT
public:
    explicit StatusLed(QWidget *parent = nullptr);

    void setColor(const QColor &color);   // 设置灯颜色并常亮
    void setOff();                        // 熄灭（灰色）
    void setBlinking(bool on);            // 闪烁开关（用于录制中）

    QSize sizeHint() const override { return {18, 18}; }
    QSize minimumSizeHint() const override { return {18, 18}; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor color = QColor(90, 90, 90);
    bool blinking = false;
    bool blinkOn = true;
    QTimer blinkTimer;
};

#endif // STATUSLED_H
