#include "VideoWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QMouseEvent>

VideoWidget::VideoWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(240, 180);
    setAutoFillBackground(false);
    setCursor(Qt::PointingHandCursor); // 提示可点击（双击放大）
}

void VideoWidget::setCameraName(const QString &name)
{
    camName = name;
    update();
}

void VideoWidget::updateFrame(const QImage &image)
{
    frame = image;          // QImage 隐式共享，拷贝开销小
    update();               // 触发 paintEvent（UI 线程内执行，线程安全）
}

void VideoWidget::setMotionState(bool detected)
{
    motionDetected = detected;
    update();
}

void VideoWidget::setRecordingState(bool rec)
{
    recording = rec;
    update();
}

void VideoWidget::clearFrame()
{
    frame = QImage();
    motionDetected = false;
    recording = false;
    update();
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit zoomRequested();
    QWidget::mouseDoubleClickEvent(event);
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // ---- 背景 ----
    p.fillRect(rect(), QColor(16, 16, 16));

    // ---- 视频帧：保持宽高比居中绘制 ----
    if (!frame.isNull())
    {
        QImage scaled = frame.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPoint topLeft((width() - scaled.width()) / 2, (height() - scaled.height()) / 2);
        p.drawImage(topLeft, scaled);
    }
    else
    {
        p.setPen(QColor(90, 90, 90));
        p.drawText(rect(), Qt::AlignCenter,
                   camName.isEmpty() ? QStringLiteral("空闲")
                                     : QStringLiteral("无视频信号"));
    }

    QFont boldFont = p.font();
    boldFont.setBold(true);

    // ---- 叠加：摄像头名称（左上角，半透明底）----
    if (!camName.isEmpty())
    {
        p.setFont(boldFont);
        QRect nameRect = p.fontMetrics().boundingRect(camName);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 140));
        p.drawRoundedRect(6, 6, nameRect.width() + 16, nameRect.height() + 10, 4, 4);
        p.setPen(QColor(230, 230, 230));
        p.drawText(14, 6 + nameRect.height() + 1, camName);
    }

    // ---- 叠加：REC 录制标志（名称下方，红点+文字）----
    if (recording)
    {
        int recY = camName.isEmpty() ? 12 : 34;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(220, 40, 40));
        p.drawEllipse(12, recY, 12, 12);
        p.setPen(QColor(220, 40, 40));
        p.setFont(boldFont);
        p.drawText(30, recY + 11, QStringLiteral("REC"));
    }

    // ---- 叠加：运动状态文字（右上角）----
    if (!frame.isNull())
    {
        QString text = motionDetected ? QStringLiteral("● 检测到运动")
                                      : QStringLiteral("○ 画面正常");
        QColor color = motionDetected ? QColor(255, 200, 40) : QColor(80, 200, 80);

        p.setFont(boldFont);
        QRect textRect = p.fontMetrics().boundingRect(text);
        int x = width() - textRect.width() - 14;
        int y = 22;

        // 半透明底色，保证文字在亮画面上也可读
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 140));
        p.drawRoundedRect(x - 8, y - textRect.height() - 4,
                          textRect.width() + 16, textRect.height() + 10, 4, 4);
        p.setPen(color);
        p.drawText(x, y, text);
    }

    // ---- 边框：颜色随状态变化 ----
    QColor border(60, 60, 60);              // 空闲
    if (!frame.isNull())
        border = recording ? QColor(220, 40, 40)      // 录制中：红
                 : motionDetected ? QColor(255, 200, 40)  // 运动：黄
                                  : QColor(80, 200, 80);  // 正常：绿
    p.setPen(QPen(border, 2));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect().adjusted(1, 1, -1, -1));
}
