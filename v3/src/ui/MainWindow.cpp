#include "MainWindow.h"
#include "VideoWidget.h"
#include "../core/VideoThread.h"

#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStackedLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QStatusBar>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    cameraSlots.resize(MAX_CELLS);
    buildUi();
    updateSummary();
}

MainWindow::~MainWindow()
{
    onStopAll();
}

// ============ UI 搭建 ============

void MainWindow::buildUi()
{
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(buildVideoArea());
    splitter->addWidget(buildControlPanel());
    splitter->setStretchFactor(0, 4);
    splitter->setStretchFactor(1, 1);
    splitter->setCollapsible(1, false);
    setCentralWidget(splitter);

    statusLabel = new QLabel(QStringLiteral("0/9 路"));
    statusBar()->addPermanentWidget(statusLabel);
    statusBar()->showMessage(QStringLiteral("就绪"));
}

// 左侧：QStackedLayout 两页——page0 九宫格，page1 单路放大
QWidget *MainWindow::buildVideoArea()
{
    // page0：九宫格
    auto *gridPage = new QWidget;
    gridLayout = new QGridLayout(gridPage);
    gridLayout->setSpacing(4);
    gridLayout->setContentsMargins(4, 4, 4, 4);

    for (int i = 0; i < MAX_CELLS; ++i)
    {
        auto *cell = new VideoWidget;
        cell->setCameraName(QString("CAM %1").arg(i + 1));
        gridLayout->addWidget(cell, i / 3, i % 3);
        cameraSlots[i].widget = cell;

        // 双击格子 -> 放大/还原
        connect(cell, &VideoWidget::zoomRequested, this,
                [this, i] { toggleZoom(i); });
    }

    // page1：放大页（运行时把对应格子 reparent 进来）
    zoomPage = new QWidget;
    auto *zoomLayout = new QVBoxLayout(zoomPage);
    zoomLayout->setContentsMargins(0, 0, 0, 0);

    auto *host = new QWidget;
    videoStack = new QStackedLayout(host);
    videoStack->addWidget(gridPage);
    videoStack->addWidget(zoomPage);
    return host;
}

QWidget *MainWindow::buildControlPanel()
{
    auto *panel = new QWidget;
    panel->setMinimumWidth(260);
    panel->setMaximumWidth(340);
    auto *panelLayout = new QVBoxLayout(panel);

    // ---- 组1：视频源 ----
    auto *sourceGroup = new QGroupBox(QStringLiteral("视频源"));
    auto *sourceLayout = new QVBoxLayout(sourceGroup);

    sourceEdit = new QLineEdit;
    sourceEdit->setPlaceholderText(QStringLiteral("视频路径 / RTSP 地址 / 0=摄像头"));

    addBtn = new QPushButton(QStringLiteral("添加并启动"));
    stopAllBtn = new QPushButton(QStringLiteral("全部停止"));

    sourceLayout->addWidget(sourceEdit);
    sourceLayout->addWidget(addBtn);
    sourceLayout->addWidget(stopAllBtn);

    // ---- 组2：检测参数（全局，作用于所有已连接路）----
    auto *paramGroup = new QGroupBox(QStringLiteral("检测参数（全局）"));
    auto *paramLayout = new QVBoxLayout(paramGroup);

    auto *sliderRow = new QHBoxLayout;
    sensitivitySlider = new QSlider(Qt::Horizontal);
    sensitivitySlider->setRange(0, 100);
    sensitivitySlider->setValue(50);
    sensitivitySlider->setTickPosition(QSlider::TicksBelow);
    sensitivitySlider->setTickInterval(10);
    sensitivityValueLabel = new QLabel(QStringLiteral("50%"));
    sensitivityValueLabel->setMinimumWidth(40);
    sensitivityValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    sliderRow->addWidget(new QLabel(QStringLiteral("灵敏度")));
    sliderRow->addWidget(sensitivitySlider, 1);
    sliderRow->addWidget(sensitivityValueLabel);
    paramLayout->addLayout(sliderRow);

    // ---- 组3：运行状态汇总 ----
    auto *statusGroup = new QGroupBox(QStringLiteral("运行状态"));
    auto *statusForm = new QFormLayout(statusGroup);

    connectedLabel = new QLabel(QStringLiteral("0/9 路"));
    motionCountLabel = new QLabel(QStringLiteral("0 路"));
    recordCountLabel = new QLabel(QStringLiteral("0 路"));
    avgFpsLabel = new QLabel(QStringLiteral("—"));

    statusForm->addRow(QStringLiteral("已连接"), connectedLabel);
    statusForm->addRow(QStringLiteral("运动中"), motionCountLabel);
    statusForm->addRow(QStringLiteral("录制中"), recordCountLabel);
    statusForm->addRow(QStringLiteral("平均帧率"), avgFpsLabel);

    // ---- 操作提示 ----
    auto *hintLabel = new QLabel(QStringLiteral("提示：双击格子放大，Esc 返回九宫格"));
    hintLabel->setStyleSheet("color:#888; font-size:11px;");
    hintLabel->setWordWrap(true);

    // ---- 组装 ----
    panelLayout->addWidget(sourceGroup);
    panelLayout->addWidget(paramGroup);
    panelLayout->addWidget(statusGroup);
    panelLayout->addWidget(hintLabel);
    panelLayout->addStretch(1);

    // ---- 信号连接 ----
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddCamera);
    connect(stopAllBtn, &QPushButton::clicked, this, &MainWindow::onStopAll);
    connect(sensitivitySlider, &QSlider::valueChanged, this, &MainWindow::onSensitivityChanged);
    connect(sourceEdit, &QLineEdit::returnPressed, addBtn, &QPushButton::click);

    return panel;
}

// ============ 多路管理（v3 联调时由成员C 的 CameraManager 接管）============

int MainWindow::findFreeSlot() const
{
    for (int i = 0; i < MAX_CELLS; ++i)
        if (!cameraSlots[i].thread)
            return i;
    return -1;
}

int MainWindow::slotIndexOf(VideoThread *thread) const
{
    for (int i = 0; i < MAX_CELLS; ++i)
        if (cameraSlots[i].thread == thread)
            return i;
    return -1;
}

void MainWindow::startSlot(int index, const QString &source)
{
    CameraSlot &slot = cameraSlots[index];

    auto *thread = new VideoThread(this);
    thread->setSource(source, index + 1);       // 路号 1~9，用于录像文件命名
    thread->setSensitivity(sensitivitySlider->value() / 100.0);

    // 帧画面直接连到对应格子（跨线程队列连接，安全）
    connect(thread, &VideoThread::frameReady, slot.widget, &VideoWidget::updateFrame);

    // 状态信号带路号捕获，更新格子 + 右侧汇总
    connect(thread, &VideoThread::motionState, this, [this, index](bool detected) {
        cameraSlots[index].motion = detected;
        cameraSlots[index].widget->setMotionState(detected);
        updateSummary();
    });
    connect(thread, &VideoThread::recordingState, this, [this, index](bool rec) {
        cameraSlots[index].recording = rec;
        cameraSlots[index].widget->setRecordingState(rec);
        updateSummary();
    });
    connect(thread, &VideoThread::fpsStat, this, [this, index](double fps) {
        cameraSlots[index].fps = fps;
        updateSummary();
    });

    // 结束/出错：用 sender() 反查路号，统一清理
    connect(thread, &VideoThread::finished, this, [this]() {
        int i = slotIndexOf(qobject_cast<VideoThread *>(sender()));
        if (i >= 0)
        {
            statusBar()->showMessage(QString("CAM %1 视频源已结束").arg(i + 1), 3000);
            stopSlot(i);
        }
    });
    connect(thread, &VideoThread::finishedWithError, this, [this](const QString &msg) {
        int i = slotIndexOf(qobject_cast<VideoThread *>(sender()));
        statusBar()->showMessage(QString("CAM %1 错误: %2").arg(i + 1).arg(msg), 5000);
        if (i >= 0)
            stopSlot(i);
    });

    slot.thread = thread;
    slot.source = source;
    slot.fps = 0.0;
    slot.motion = false;
    slot.recording = false;

    thread->start();
    updateSummary();
    statusBar()->showMessage(QString("CAM %1 已连接: %2").arg(index + 1).arg(source), 3000);
}

void MainWindow::stopSlot(int index)
{
    CameraSlot &slot = cameraSlots[index];
    if (!slot.thread)
        return;

    // 若该路正处于放大状态，先退回九宫格，避免控件归属混乱
    if (zoomedIndex == index)
        exitZoom();

    slot.thread->stop();
    slot.thread->wait();            // 等线程安全退出（含关闭录像文件）
    slot.thread->deleteLater();
    slot.thread = nullptr;

    slot.widget->clearFrame();
    slot.fps = 0.0;
    slot.motion = false;
    slot.recording = false;
    updateSummary();
}

// ============ 用户操作 ============

void MainWindow::onAddCamera()
{
    QString src = sourceEdit->text().trimmed();
    if (src.isEmpty())
    {
        statusBar()->showMessage(QStringLiteral("请输入视频路径或 0（摄像头）"), 3000);
        return;
    }

    int index = findFreeSlot();
    if (index < 0)
    {
        statusBar()->showMessage(QStringLiteral("已满 9 路，无法继续添加"), 3000);
        return;
    }

    startSlot(index, src);
    sourceEdit->clear();
}

void MainWindow::onStopAll()
{
    exitZoom();
    for (int i = 0; i < MAX_CELLS; ++i)
        stopSlot(i);
    statusBar()->showMessage(QStringLiteral("已全部停止"), 3000);
}

void MainWindow::onSensitivityChanged(int value)
{
    sensitivityValueLabel->setText(QString("%1%").arg(value));
    // 全局灵敏度：应用到所有正在运行的路
    for (auto &slot : cameraSlots)
        if (slot.thread)
            slot.thread->setSensitivity(value / 100.0);
}

// ============ 双击放大 / Esc 返回 ============

void MainWindow::toggleZoom(int index)
{
    if (zoomedIndex == index)
    {
        exitZoom();
        return;
    }

    exitZoom(); // 先还原因其他格子而处于放大状态的控件

    // 把格子从九宫格 reparent 到放大页
    VideoWidget *cell = cameraSlots[index].widget;
    zoomPage->layout()->addWidget(cell);
    cell->show();                   // reparent 后确保可见
    videoStack->setCurrentWidget(zoomPage);
    zoomedIndex = index;
    statusBar()->showMessage(QString("CAM %1 放大中，Esc 返回").arg(index + 1));
}

void MainWindow::exitZoom()
{
    if (zoomedIndex < 0)
        return;

    // 把格子放回九宫格原位置
    VideoWidget *cell = cameraSlots[zoomedIndex].widget;
    gridLayout->addWidget(cell, zoomedIndex / 3, zoomedIndex % 3);
    cell->show();                   // reparent 后需重新 show
    videoStack->setCurrentIndex(0);
    zoomedIndex = -1;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && zoomedIndex >= 0)
    {
        exitZoom();
        statusBar()->showMessage(QStringLiteral("已返回九宫格"), 2000);
        return;
    }
    QMainWindow::keyPressEvent(event);
}

// ============ 状态汇总 ============

void MainWindow::updateSummary()
{
    int connected = 0, motion = 0, recording = 0;
    double fpsSum = 0.0;

    for (const auto &slot : cameraSlots)
    {
        if (!slot.thread)
            continue;
        ++connected;
        if (slot.motion) ++motion;
        if (slot.recording) ++recording;
        fpsSum += slot.fps;
    }

    connectedLabel->setText(QString("%1/9 路").arg(connected));
    motionCountLabel->setText(QString("%1 路").arg(motion));
    motionCountLabel->setStyleSheet(motion > 0 ? "color:#e6a23c; font-weight:bold;" : "");
    recordCountLabel->setText(QString("%1 路").arg(recording));
    recordCountLabel->setStyleSheet(recording > 0 ? "color:#dc2828; font-weight:bold;" : "");
    avgFpsLabel->setText(connected > 0
                             ? QString("%1 FPS").arg(fpsSum / connected, 0, 'f', 1)
                             : QStringLiteral("—"));
    statusLabel->setText(QString("%1/9 路").arg(connected));
}
