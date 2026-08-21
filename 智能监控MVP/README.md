# 智能监控 MVP（运动检测版）

> 从「实时智能监控产品原型」完整方案里砍出来的**最小可运行版本**。目标：2 周内跑通 + 能演示 + 能写进简历，而不是再产出一份半成品方案。

## 一、项目定位

- **完整版**（见 [[系统编程项目/智能监控-技术架构1]] / [[系统编程项目/智能监控-技术架构2]]）：9 路 RTSP、VLC 转码、H.264 存储、SQLite、QML 九宫格 —— 太重，不适合第一个项目。
- **本 MVP**：本地视频文件 / 单个摄像头 → OpenCV 运动检测 → Qt 界面实时显示。**去掉 VLC / RTSP / QML / 转码存储**，先跑通核心链路。

## 二、技术栈（简历关键词）

`C++17 · Qt(Widgets) · OpenCV(MOG2 运动检测) · 多线程(QThread) · 信号槽`

## 三、架构

```
┌─────────────┐   读帧    ┌──────────────────┐   frameReady 信号   ┌──────────────┐
│ 视频源      │ ────────▶ │ VideoThread 子线程 │ ─────────────────▶ │ MainWindow UI │
│ 文件/摄像头  │           │ 运动检测+画框       │  (队列连接，线程安全) │  QLabel 显示   │
└─────────────┘           └──────────────────┘                    └──────────────┘
```

- **VideoThread**（子线程）：读帧 → MotionDetector 运动检测 → 转 QImage → 发信号，绝不阻塞 UI。
- **MotionDetector**：MOG2 背景减除 + 形态学去噪 + 轮廓检测 + 画矩形框。
- **MainWindow**（主线程）：只负责显示和交互。

## 四、目录结构

```
智能监控MVP/
├── README.md
├── CMakeLists.txt
└── src/
    ├── main.cpp
    ├── MainWindow.h / .cpp      # 界面
    ├── VideoThread.h / .cpp     # 采集线程
    └── MotionDetector.h / .cpp  # 运动检测
```

## 五、环境准备（Ubuntu / WSL）

```bash
sudo apt update
sudo apt install build-essential cmake
sudo apt install qt6-base-dev libopencv-dev   # Qt6 + OpenCV
# 若只有 Qt5：sudo apt install qtbase5-dev libopencv-dev
```

## 六、编译运行

```bash
cd 智能监控MVP
mkdir build && cd build
cmake ..
make -j4
./SmartSurveillanceMVP
```

## 七、使用

- 视频源输入框填 `0` = 默认摄像头；填文件路径 = 播放本地视频（如 `./test.mp4`）。
- 点「开始」，界面上会实时显示视频，检测到运动物体时画面出现**绿色矩形框**，状态栏提示"检测到运动"。

## 八、迭代路线

| 版本 | 增加什么 |
|------|---------|
| **v1（本版本）** | 单路视频 + MOG2 运动检测 + Qt 显示 |
| v2 | 运动时自动录制片段（`cv::VideoWriter`） |
| v3 | 多路视频 + 多线程池；预留 RTSP 接口 |
| v4 | 接回完整方案：RTSP + VLC 转码 + 存储管理 |

## 九、简历亮点（做完后这样写）

> 基于 C++17 + Qt + OpenCV 实现实时智能监控系统：QThread 子线程采集视频、MOG2 背景减除 + 形态学 + 轮廓检测实现运动目标检测并绘制边界框，通过 Qt 信号槽（队列连接）线程安全地实时刷新界面，采集线程与 UI 线程分离避免卡顿。

## 十、关键知识点回顾

- **线程安全**：子线程用 `emit frameReady(QImage)` 发信号，跨线程默认队列连接，UI 在主线程更新 —— 这是 Qt 多线程的标准姿势。
- **OpenCV 到 QImage**：注意 BGR→RGB 转换 + `.copy()` 深拷贝，否则 QImage 指向已释放的内存。
- **运动检测**：MOG2 背景建模 + 形态学开/闭运算去噪 + 轮廓面积阈值过滤小噪点。

## 相关笔记
- 完整方案：[[系统编程项目/智能监控-技术架构1]]、[[系统编程项目/智能监控-技术架构2]]
- 求职规划：[[能做的项目清单]]
- 线程知识：[[0729]]、[[0730]]
