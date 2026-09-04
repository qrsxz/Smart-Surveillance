#include "MainWindow.h"
#include "VideoThread.h"
#include "StreamServer.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QPixmap>
#include <QNetworkInterface>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    // ---- 控制栏 ----
    sourceEdit = new QLineEdit;
    sourceEdit->setPlaceholderText("视频路径 / RTSP 地址 / 0=摄像头，如 /home/suge/08021/test.mp4");
    sourceEdit->setMinimumWidth(360);

    addBtn = new QPushButton("添加并启动");
    stopAllBtn = new QPushButton("全部停止");
    countingCheckBox = new QCheckBox("显示人物计数");
    countingCheckBox->setChecked(true);  // 默认开启
    infoLabel = new QLabel("已添加 0/9 路");
    infoLabel->setStyleSheet("color:#888;");

    auto *top = new QHBoxLayout;
    top->addWidget(sourceEdit, 1);
    top->addWidget(addBtn);
    top->addWidget(stopAllBtn);
    top->addWidget(countingCheckBox);
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
    connect(countingCheckBox, &QCheckBox::toggled, this, &MainWindow::onToggleCounting);

    // v5: 启动局域网流媒体服务
    startStreamServer();
}

MainWindow::~MainWindow()
{
    onStopAll();
}

// v5: 启动局域网流媒体服务，浏览器访问 http://IP:8081 看实时画面
void MainWindow::startStreamServer()
{
    streamServer = new StreamServer(this);
    if (streamServer->startServer(8081))
    {
        QString ip = localIpAddress();
        qInfo() << "局域网监控地址: http://" << ip << ":8081";
        infoLabel->setText(QString("已添加 0/9 路 | 局域网: http://%1:8081").arg(ip));
    }
    else
    {
        qWarning() << "流媒体服务器启动失败: 端口 8081 可能被占用";
    }
}

// v5: 获取本机第一个可用的局域网 IPv4 地址
QString MainWindow::localIpAddress() const
{
    const QList<QHostAddress> addrs = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr : addrs)
    {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
            !addr.isLoopback() && addr != QHostAddress::LocalHost)
        {
            return addr.toString();
        }
    }
    return "127.0.0.1";
}

// v4: 切换所有视频路的计数功能
void MainWindow::onToggleCounting(bool checked)
{
    for (auto &c : cells)
    {
        if (c.thread)
        {
            c.thread->setCountingEnabled(checked);
            // 同时隐藏/显示UI上的计数标签
            if (c.countLabel)
                c.countLabel->setVisible(checked);
        }
    }
}

// v4: 添加一路视频源：创建线程 + 格子，启动线程
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
    thread->setCountingEnabled(countingCheckBox->isChecked());

    auto *nameLabel = new QLabel("加载中…");
    nameLabel->setStyleSheet("color:#aaa; padding:2px; background:#111;");
    nameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *videoLabel = new QLabel("…");
    videoLabel->setAlignment(Qt::AlignCenter);
    videoLabel->setMinimumSize(200, 120);
    videoLabel->setStyleSheet("background:#000; color:#666;");

    auto *countLabel = new QLabel("人物: 0");
    countLabel->setStyleSheet("color:#0f0; padding:2px; background:#111; font-weight:bold;");
    countLabel->setAlignment(Qt::AlignCenter);
    countLabel->setVisible(countingCheckBox->isChecked());  // 初始状态跟随checkbox

    auto *box = new QVBoxLayout;
    box->setContentsMargins(0, 0, 0, 0);
    box->setSpacing(0);
    box->addWidget(nameLabel);
    box->addWidget(videoLabel, 1);
    box->addWidget(countLabel);

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
            [this, videoLabel](const QImage &img) {
                videoLabel->setPixmap(QPixmap::fromImage(img)
                                          .scaled(videoLabel->size(),
                                                  Qt::KeepAspectRatio,
                                                  Qt::SmoothTransformation));
                // v5: 同步推流到局域网（内部无客户端时会自动跳过）
                if (streamServer)
                    streamServer->broadcastFrame(img);
            });
    connect(thread, &VideoThread::motionState, this,
            [nameLabel](bool detected) {
                nameLabel->setText(detected ? "● 检测到人物" : "○ 无人");
            });
    connect(thread, &VideoThread::personCountChanged, this,
            [countLabel](int count) {
                countLabel->setText(QString("人物: %1").arg(count));
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

    cells.append(CameraCell{thread, videoLabel, nameLabel, countLabel});
    infoLabel->setText(QString("已添加 %1/9 路 | 局域网: http://%2:8081")
                           .arg(cells.size())
                           .arg(localIpAddress()));

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

// v4: 停止并清理所有路
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

    infoLabel->setText("已添加 0/9 路 | 局域网: http://" + localIpAddress() + ":8081");
}
