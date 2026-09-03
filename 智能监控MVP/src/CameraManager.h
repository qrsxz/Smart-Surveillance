#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <QObject>
#include <QVector>
#include <QImage>

class VideoThread;

// v4: 摄像头管理类：统一创建/启动/停止 VideoThread，按路号转发信号。
// 职责：线程生命周期 + 信号分发。UI 只通过本类与线程交互，不直接接触线程。
class CameraManager : public QObject
{
    Q_OBJECT
public:
    explicit CameraManager(QObject *parent = nullptr);
    ~CameraManager();

    // 添加一路视频源（文件路径 / RTSP 地址 / 0=摄像头），返回路号 id；达上限返回 -1
    int addCamera(const QString &source);
    // 停止并清理所有路
    void stopAll();
    int count() const { return cams.size(); }

    static const int MAX_CAMERAS = 9;

signals:
    void frameReady(int id, const QImage &image);
    void motionState(int id, bool detected);
    void recordingState(int id, bool recording);
    void cameraError(int id, const QString &msg);
    void cameraFinished(int id);

private:
    struct CameraEntry
    {
        int id = 0;
        VideoThread *thread = nullptr;
    };

    QVector<CameraEntry> cams;
    int nextId = 1;
};

#endif