# 智能监控 v3 — 成员A（UI 与交互）交付说明

## 本阶段任务完成情况

| 计划任务 | 状态 | 说明 |
|---------|------|------|
| ① 九宫格布局（QGridLayout，3×3） | ✅ | 9 个格子预创建，添加视频源自动填入空闲格 |
| ② 每个格子是自定义 VideoWidget | ✅ | 独立控件，QPainter 绘制，一套信号槽管一路 |
| ③ 双击格子放大，Esc 返回 | ✅ | `QStackedLayout` 双页切换（九宫格页/放大页），双击还原也可 |
| ④ 格子上叠加摄像头名称和运动标记 | ✅ | 左上角 CAM 名称 + REC 标志，右上角运动状态，边框变色 |

## 目录结构

```
v3/
├── CMakeLists.txt
├── README.md                 ← 本文件
└── src/
    ├── main.cpp
    ├── ui/                   ★ 成员A 负责
    │   ├── MainWindow.h/cpp      九宫格主窗口 + 放大切换 + 多路状态汇总
    │   ├── VideoWidget.h/cpp     格子控件（名称/REC/运动叠加 + 双击放大信号）
    │   └── StatusLed.h/cpp       （v2 遗留，v3 面板改用汇总数字，v4 设置页会复用）
    ├── core/                 ◆ 成员C（过渡版，成员A临时维护）
    │   └── VideoThread.h/cpp     与 v2 相同：fpsStat + setSensitivity
    └── algorithm/            ◆ 成员B（过渡版）
        └── MotionDetector.h/cpp  与 v2 相同：setSensitivity 接口
```

## 多路管理说明（与成员C 的联调边界）

当前 `MainWindow` 内部用 `QVector<CameraSlot>` 管理 9 个 `VideoThread`（每路一个线程），
包含：空闲槽查找、按路号启动/停止、结束/出错自动清理。
**v3 联调时这部分应整体替换为成员C 的 `CameraManager`**，UI 侧只需要保留：

```cpp
// UI 依赖的 per-camera 信号（CameraManager 转发或直接暴露 VideoThread 均可）
frameReady(QImage) / motionState(bool) / recordingState(bool) / fpsStat(double)
finished() / finishedWithError(QString)

// UI 调用
setSource(QString, int id) / setSensitivity(double) / start() / stop()
```

替换点集中在 `MainWindow::startSlot / stopSlot / slotIndexOf` 三个函数。

## 放大/还原实现

- 双击格子 → `VideoWidget::zoomRequested()` → `MainWindow::toggleZoom(index)`
- 放大 = 把该格子的 `VideoWidget` 从 `QGridLayout` reparent 到放大页容器，`QStackedLayout` 切页
- Esc / 再次双击 → reparent 回九宫格原位置
- 放大状态下该路视频源结束，会先自动退回九宫格再清理，避免控件归属混乱

## 编译运行

```bash
cd v3
mkdir build && cd build
cmake ..
make -j$(nproc)
./SmartSurveillanceV3
```

测试多路（无真实摄像头时）：
- 输入同一个视频文件路径，点多次“添加并启动”，即可模拟多路
- 或准备多段不同视频分别添加
- 每路独立运动检测、独立自动录制，录像在 `recordings/cam{路号}_时间戳.mp4`

已在 Ubuntu 24.04 + Qt 6.4.2 + OpenCV 4.6 编译并运行验证：
3 路并发、运动检测画框、运动触发自动录制、帧率统计均正常。

## 已知限制（留给 v4 / 联调）

- 灵敏度是全局的（一个滑块控制所有路）；如需每路独立调节，在 v4 设置界面做
- 没有单路移除按钮（只有“全部停止”）；v4 可在格子上加右键菜单
- 9 路并发性能取决于成员B 的算法优化（降分辨率/跳帧）和成员C 的线程池，
  UI 侧已做到每帧零拷贝绘制（QImage 隐式共享 + QPainter 直接渲染）
