#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QString>

class QLineEdit;
class QPushButton;
class QSlider;
class QLabel;
class QGridLayout;
class QStackedLayout;
class QKeyEvent;
class VideoWidget;
class VideoThread;

// v3 成员A：主窗口——九宫格多路监控
//   左侧：QStackedLayout 两页——九宫格页 / 单路放大页（双击格子切换，Esc 返回）
//   右侧：控制面板（添加视频源 / 全局灵敏度 / 多路运行状态汇总）
// 每一路 = 一个 VideoWidget（格子）+ 一个 VideoThread（采集线程），
// 多路线程管理目前由本类承担，v3 联调时替换为成员C 的 CameraManager
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddCamera();                         // 添加一路视频源
    void onStopAll();                           // 全部停止
    void onSensitivityChanged(int value);       // 全局灵敏度滑块

private:
    // ---- 一路视频的完整状态 ----
    struct CameraSlot
    {
        VideoThread *thread = nullptr;
        VideoWidget *widget = nullptr;          // 预创建的格子控件
        QString source;
        double fps = 0.0;
        bool motion = false;
        bool recording = false;
    };

    void buildUi();
    QWidget *buildVideoArea();                  // 左侧：九宫格 + 放大页
    QWidget *buildControlPanel();               // 右侧控制面板

    void startSlot(int index, const QString &source);  // 启动指定路
    void stopSlot(int index);                          // 停止指定路
    int findFreeSlot() const;                          // 找第一个空闲格子
    int slotIndexOf(VideoThread *thread) const;        // 由线程反查路号

    void toggleZoom(int index);                 // 双击：放大/还原
    void exitZoom();
    void updateSummary();                       // 刷新右侧状态汇总

    void keyPressEvent(QKeyEvent *event) override;     // Esc 退出放大

    static const int MAX_CELLS = 9;             // 九宫格上限

    // ---- 左侧视频区 ----
    QStackedLayout *videoStack = nullptr;       // 九宫格页 / 放大页
    QGridLayout *gridLayout = nullptr;          // 3×3 格子布局
    QWidget *zoomPage = nullptr;                // 放大页容器
    int zoomedIndex = -1;                       // 当前放大的路号，-1=未放大

    // ---- 右侧控制面板 ----
    QLineEdit *sourceEdit = nullptr;
    QPushButton *addBtn = nullptr;
    QPushButton *stopAllBtn = nullptr;
    QSlider *sensitivitySlider = nullptr;
    QLabel *sensitivityValueLabel = nullptr;
    QLabel *connectedLabel = nullptr;           // 已连接 x/9
    QLabel *motionCountLabel = nullptr;         // 运动中 x 路
    QLabel *recordCountLabel = nullptr;         // 录制中 x 路
    QLabel *avgFpsLabel = nullptr;              // 平均帧率
    QLabel *statusLabel = nullptr;              // 状态栏

    QVector<CameraSlot> cameraSlots;            // 9 路槽位（注意：勿命名为 slots，与 Qt 关键字冲突）
};

#endif // MAINWINDOW_H
