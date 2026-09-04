# 智能监控系统（Smart Surveillance）

基于 **C++17 + Qt + OpenCV** 的多路实时监控系统：支持本地视频 / 摄像头 / RTSP 多路接入，采用 **HOG + SVM 行人检测** 实时标注与计数，人物触发自动录像，并内置 **MJPEG 局域网推流**——同一 WiFi 下手机、平板、电脑用浏览器即可实时观看，无需安装任何软件。

## ✨ 功能特性

- **HOG + SVM 行人检测**：`cv::HOGDescriptor` + 预训练 INRIA 行人模型，`detectMultiScale` 多尺度检测 + `NMSBoxes` 非极大值抑制去重，只识别"人"，过滤树叶、光影、动物等运动干扰
- **多路九宫格并发**：最多 9 路视频源，每路一个独立 `QThread` 采集线程，互不阻塞
- **人物计数 + 置信度**：实时显示 `Person Count: N` 与每个框的 `Person: 0.XX` 置信度，支持运行时动态开关
- **人物触发自动录像**：检测到人物自动录制（`cv::VideoWriter`），连续 3 秒无人自动停止，多路文件名带路号避免互相覆盖
- **MJPEG 局域网推流**：基于 `QTcpServer` 实现 `multipart/x-mixed-replace` 流，浏览器访问 `http://<IP>:8081` 即看；无客户端时跳过编码，零额外 CPU 开销
- **线程安全 UI**：子线程 `emit frameReady(QImage)` 信号，跨线程队列连接，UI 主线程刷新，界面流畅不卡顿

## 🛠 技术栈

`C++17` · `Qt5/Qt6 (Widgets + Network)` · `OpenCV 4.x (objdetect / dnn / videoio)` · `CMake` · `QThread 多线程` · `MJPEG over HTTP`

## 🏗 架构

```
┌──────────────┐   视频源        ┌─────────────────────────────────┐
│ 摄像头 / 文件 │ ─────────────▶ │ VideoThread (QThread 子线程)      │
│ / RTSP       │   每路一个线程   │  读帧 → 行人检测 → 运动自动录像      │
└──────────────┘                └───────────────┬─────────────────┘
                                                 │ frameReady(QImage) 信号
                                                 │ (队列连接，跨线程安全)
                                   ┌─────────────▼──────────────────┐
                                   │ MainWindow (UI 主线程)           │
                                   │  九宫格显示 + 计数 + 状态标签      │────▶ StreamServer
                                   └─────────────────────────────────┘      MJPEG 推流 :8081
                                                                              └▶ 浏览器实时观看
```

- **VideoThread**（子线程，每路一个）：读帧 → `PersonDetector` 行人检测 → 运动防抖 → 自动录像 → `Mat→QImage` → `emit frameReady`，绝不阻塞 UI
- **PersonDetector**：HOG + SVM 行人检测，画框 + 置信度 + 计数
- **StreamServer**（v5 新增）：MJPEG over HTTP 局域网推流，无客户端时零开销
- **MainWindow**（主线程）：九宫格布局 + 控制栏 + 推流调度，只负责显示与交互

## 📁 目录结构

```
智能监控MVP/
├── README.md
├── CMakeLists.txt
└── src/
    ├── main.cpp                 # 入口
    ├── MainWindow.h / .cpp      # 主界面（九宫格 + 推流调度）
    ├── VideoThread.h / .cpp     # 采集线程（读帧/检测/录像）
    ├── PersonDetector.h / .cpp  # HOG+SVM 行人检测器
    └── StreamServer.h / .cpp    # MJPEG 局域网推流服务器
```

## 🚀 编译运行

### 环境准备（Ubuntu 22.04 / WSL）

```bash
sudo apt update
sudo apt install build-essential cmake
sudo apt install qtbase5-dev libopencv-dev   # Qt5 + OpenCV4
# 或 Qt6：sudo apt install qt6-base-dev libopencv-dev
```

### 编译

```bash
cd 智能监控MVP
mkdir -p build && cd build
cmake ..
make -j4
./SmartSurveillanceMVP
```

程序启动后，窗口底部会显示局域网访问地址，如 `局域网: http://192.168.1.100:8081`。

## 🎯 关键实现

### 1. 行人检测（PersonDetector）

```cpp
hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());          // INRIA 预训练模型
hog.detectMultiScale(frame, found, weights, 0, cv::Size(8,8), cv::Size(0,0), 1.05, 2, false); // 多尺度
cv::dnn::NMSBoxes(found, weightsFloat, 0.5, 0.4, indices);                   // NMS 去重
```

- HOG（方向梯度直方图）提取特征，SVM 分类，只识别"人形"目标
- `detectMultiScale` 滑动窗口多尺度检测，`NMSBoxes` 合并重叠框
- 每帧检测后绘制绿色边界框 + 置信度标签 + 计数

### 2. 采集线程（VideoThread）

- 每路视频一个 `QThread`，本地视频按自身 FPS 延时、文件播完自动循环
- **运动防抖**：连续 3 帧判定有/无人，状态变化时才发信号，避免状态栏狂闪
- **自动录像**：检测到人物触发 `cv::VideoWriter`（mp4v），连续 3 秒无人自动 release
- `Mat→QImage`：注意 BGR→RGB 转换 + `.copy()` 深拷贝，避免悬空指针

### 3. 局域网推流（StreamServer）

- 基于 `QTcpServer`，`multipart/x-mixed-replace` 持续推送 JPEG 帧
- **无客户端时直接 return，不编码**，零额外 CPU 开销
- 帧率上限 10fps（100ms 间隔）、缩放到 640px 宽、JPEG 质量 70，控制 CPU/带宽
- 支持多客户端同时观看

## 📈 迭代历史

| 版本 | 内容 |
|------|------|
| v1 | 单路视频 + MOG2 运动检测 + Qt 显示 |
| v2 | 运动自动录像（cv::VideoWriter） |
| v3 | 多路视频九宫格 + 多线程，预留 RTSP |
| v4 | 改用 HOG + SVM 行人检测 + 人物计数 + 置信度显示 |
| v5 | MJPEG 局域网推流（浏览器免装软件实时观看） |

## ⚠️ 已知限制

- HOG 检测器对侧面/背面/远距离小目标人物识别率较低
- 严重遮挡时识别效果下降
- 实时性受 CPU 性能影响，高分辨率多路场景可能卡顿
- 推流为 MJPEG（逐帧 JPEG），非 H.264，码率较高，仅适合局域网
