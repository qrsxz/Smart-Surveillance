#include "CameraManager.h"
#include "VideoThread.h"

CameraManager::CameraManager(QObject *parent) : QObject(parent) {}

CameraManager::~CameraManager()
{
    stopAll();
}

int CameraManager::addCamera(const QString &source)
{
    if (cams.size() >= MAX_CAMERAS)
        return -1;

    auto *thread = new VideoThread(this);
    thread->setSource(source, nextId);
    int id = nextId++;

    // 线程信号 -> 带 id 转发给 UI
    connect(thread, &VideoThread::frameReady, this,
            [this, id](const QImage &img) { emit frameReady(id, img); });
    connect(thread, &VideoThread::motionState, this,
            [this, id](bool d) { emit motionState(id, d); });
    connect(thread, &VideoThread::recordingState, this,
            [this, id](bool r) { emit recordingState(id, r); });
    connect(thread, &VideoThread::finishedWithError, this,
            [this, id](const QString &m) { emit cameraError(id, m); });
    connect(thread, &VideoThread::finished, this,
            [this, id]() { emit cameraFinished(id); });

    cams.append(CameraEntry{id, thread});
    thread->start();
    return id;
}

void CameraManager::stopAll()
{
    for (auto &e : cams)
    {
        if (e.thread)
        {
            e.thread->stop();
            e.thread->wait();
            e.thread->deleteLater();
        }
    }
    cams.clear();
    nextId = 1;
}