#include "StatusLed.h"

#include <QPainter>
#include <QPen>

StatusLed::StatusLed(QWidget *parent) : QWidget(parent)
{
    setFixedSize(18, 18);

    // 闪烁节奏：500ms 一次
    blinkTimer.setInterval(500);
    connect(&blinkTimer, &QTimer::timeout, this, [this] {
        blinkOn = !blinkOn;
        update();
    });
}

void StatusLed::setColor(const QColor &c)
{
    color = c;
    blinking = false;
    blinkTimer.stop();
    blinkOn = true;
    update();
}

void StatusLed::setOff()
{
    setColor(QColor(90, 90, 90));
}

void StatusLed::setBlinking(bool on)
{
    blinking = on;
    blinkOn = true;
    if (on)
        blinkTimer.start();
    else
        blinkTimer.stop();
    update();
}

void StatusLed::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor c = (blinking && !blinkOn) ? QColor(60, 60, 60) : color;

    // 外圈
    p.setPen(QPen(QColor(40, 40, 40), 1));
    p.setBrush(c.darker(130));
    p.drawEllipse(rect().adjusted(1, 1, -1, -1));

    // 内芯（高光，模拟 LED 立体感）
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawEllipse(rect().adjusted(4, 4, -4, -4));
}
