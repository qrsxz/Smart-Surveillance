#include "MainWindow.h"
#include "VideoThread.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    // ---- 控制栏 ----
    sourceEdit = new QLineEdit;
    sourceEdit->setPlaceholderText("视频路径 / RTSP 地址 / 0=摄像头，如 /home/suge/08021/test.mp4");
    sourceEdit->setMinimumWidth(360);

    addBtn = new QPushButton("添加并启动");
    stopAllBtn = new QPushButton("全部停止");
    infoLabel = new QLabel("已添加 0/9 路");
    infoLabel->setStyleSheet("color:#888;");

    auto *top = new QHBoxLayout;
    top->addWidget(sourceEdit, 1);
    top->addWidget(addBtn);
    top->addWidget(stopAllBtn);
    top->addWidget(infoLabel);

    // ---- 九宫格 ----
    auto *gridHost = new QWidget;
    gridLayout = new QGridLayout(gridHost);
    gridLayout->setSpacing(4);

    // 预置 9 个空格，显示"空闲"
    for (int i = 0; i < 9; ++i)
    {
        auto *empty = new QLabel("空闲");
        empty->setAlignment(Qt::AlignCenter);
        empty->setMinimumSize(200, 120);
        empty->setStyleSheet("background:#1a1a1a; color:#555; border:1px solid #2a2a2a;");
        gridLayout->addWidget(empty, i / 3, i % 3);
    }

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(gridHost, 1);

    connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddCamera);
    connect(stopAllBtn, &QPushButton::clicked, this, &MainWindow::onStopAll);
}

MainWindow::~MainWindow()
{
    onStopAll();
}

// v3: 添加一路视频源：创建线程 + 格子，启动线程
void MainWindow::addCell(const QString &source)
{
    if (cells.size() >= MAX_CELLS)
    {
        infoLabel->setText(QString("已满 %1 路，无法继续添加").arg(MAX_CELLS));
        return;
    }

    int cellIndex = cells.size();
    auto *thread = new VideoThread(this);
    thread->setSource(source, cameraIdCounter++);

    auto *nameLabel = new QLabel("加载中…");
    nameLabel->setStyleSheet("color:#aaa; padding:2px; background:#111;");
    nameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *videoLabel = new QLabel("…");
    videoLabel->setAlignment(Qt::AlignCenter);
    videoLabel->setMinimumSize(200, 120);
    videoLabel->setStyleSheet("background:#000; color:#666;");

    auto *box = new QVBoxLayout;
    box->setContentsMargins(0, 0, 0, 0);
    box->setSpacing(0);
    box->addWidget(nameLabel);
    box->addWidget(videoLabel, 1);

    auto *container = new QWidget;
    container->setLayout(box);
    container->setStyleSheet("border:1px solid #333;");

    // 覆盖预置的"空闲"格子
    QLayoutItem *old = gridLayout->itemAtPosition(cellIndex / 3, cellIndex % 3);
    if (old && old->widget())
        old->widget()->deleteLater();
    delete gridLayout->takeAt(cellIndex); // 移除旧 item（widget 已 deleteLater）

    gridLayout->addWidget(container, cellIndex / 3, cellIndex % 3);

    // 线程信号 -> 主线程更新对应格子（跨线程队列连接，安全）
    connect(thread, &VideoThread::frameReady, this,
            [videoLabel](const QImage &img) {
                videoLabel->setPixmap(QPixmap::fromImage(img)
                                          .scaled(videoLabel->size(),
                                                  Qt::KeepAspectRatio,
                                                  Qt::SmoothTransformation));
            });
    connect(thread, &VideoThread::motionState, this,
            [nameLabel](bool detected) {
                nameLabel->setText(detected ? "● 运动" : "○ 正常");
            });
    connect(thread, &VideoThread::recordingState, this,
            [nameLabel](bool rec) {
                if (rec) nameLabel->setText("● 录制中…");
            });
    connect(thread, &VideoThread::finishedWithError, this,
            [nameLabel](const QString &m) {
                nameLabel->setText("错误: " + m);
            });
    connect(thread, &VideoThread::finished, this,
            [nameLabel]() {
                nameLabel->setText("已结束");
            });

    cells.append(CameraCell{thread, videoLabel, nameLabel});
    infoLabel->setText(QString("已添加 %1/9 路").arg(cells.size()));

    thread->start();
}

void MainWindow::onAddCamera()
{
    QString src = sourceEdit->text().trimmed();
    if (src.isEmpty())
    {
        infoLabel->setText("请输入视频路径或 0(摄像头)");
        return;
    }
    addCell(src);
}

// v3: 停止并清理所有路
void MainWindow::onStopAll()
{
    for (auto &c : cells)
    {
        if (c.thread)
        {
            c.thread->stop();
            c.thread->wait();
            c.thread->deleteLater();
        }
    }
    cells.clear();
    cameraIdCounter = 1;

    // 清空 grid 并恢复"空闲"占位
    while (QLayoutItem *item = gridLayout->takeAt(0))
    {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }
    for (int i = 0; i < 9; ++i)
    {
        auto *empty = new QLabel("空闲");
        empty->setAlignment(Qt::AlignCenter);
        empty->setMinimumSize(200, 120);
        empty->setStyleSheet("background:#1a1a1a; color:#555; border:1px solid #2a2a2a;");
        gridLayout->addWidget(empty, i / 3, i % 3);
    }

    infoLabel->setText("已添加 0/9 路");
}