#include "MainWindow.h"
#include "VideoThread.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    videoLabel = new QLabel("等待开始…");
    videoLabel->setAlignment(Qt::AlignCenter);
    videoLabel->setMinimumSize(640, 360);
    videoLabel->setStyleSheet("background:#000; color:#fff;");

    statusLabel = new QLabel("状态：空闲");

    sourceEdit = new QLineEdit("0");
    sourceEdit->setPlaceholderText("视频文件路径，或 0(默认摄像头)");

    startBtn = new QPushButton("开始");
    stopBtn = new QPushButton("停止");
    stopBtn->setEnabled(false);

    auto *top = new QHBoxLayout;
    top->addWidget(new QLabel("视频源:"));
    top->addWidget(sourceEdit, 1);
    top->addWidget(startBtn);
    top->addWidget(stopBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(videoLabel, 1);
    layout->addWidget(statusLabel);

    thread = new VideoThread(this);
    connect(thread, &VideoThread::frameReady, this, &MainWindow::onFrame);
    connect(thread, &VideoThread::motionState, this, &MainWindow::onMotion);
    connect(thread, &VideoThread::finishedWithError, this, [this](const QString &m) {
        statusLabel->setText("错误：" + m);
        startBtn->setEnabled(true);
        stopBtn->setEnabled(false);
    });
    // 视频源正常结束（文件播完/摄像头断开）时复位按钮，避免界面卡死在"运行中"
    connect(thread, &VideoThread::finished, this, [this]() {
        startBtn->setEnabled(true);
        stopBtn->setEnabled(false);
        statusLabel->setText("状态：已停止（视频源结束）");
    });

    connect(startBtn, &QPushButton::clicked, this, &MainWindow::onStart);
    connect(stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);
}

MainWindow::~MainWindow()
{
    if (thread->isRunning())
    {
        thread->stop();
        thread->wait();
    }
}

void MainWindow::onStart()
{
    thread->setSource(sourceEdit->text().trimmed());
    thread->start();
    startBtn->setEnabled(false);
    stopBtn->setEnabled(true);
    statusLabel->setText("状态：运行中");
}

void MainWindow::onStop()
{
    thread->stop();
    thread->wait();
    startBtn->setEnabled(true);
    stopBtn->setEnabled(false);
    statusLabel->setText("状态：已停止");
}

void MainWindow::onFrame(const QImage &image)
{
    videoLabel->setPixmap(
        QPixmap::fromImage(image).scaled(videoLabel->size(),
                                         Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation));
}

void MainWindow::onMotion(bool detected)
{
    statusLabel->setText(detected ? "状态：检测到运动！" : "状态：运行中");
}