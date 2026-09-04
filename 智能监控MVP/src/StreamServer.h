#ifndef STREAMSERVER_H
#define STREAMSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QImage>
#include <QElapsedTimer>

// MJPEG 流媒体服务器
// 局域网内浏览器访问 http://<电脑IP>:8081 即可看到实时监控画面
class StreamServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit StreamServer(QObject *parent = nullptr);

    // 启动服务器，port 默认 8081
    bool startServer(quint16 port = 8081);
    // 是否有客户端正在观看
    bool hasClients() const { return !clients.isEmpty(); }

public slots:
    // 广播一帧画面（内部会缩放+编码，无客户端时自动跳过）
    void broadcastFrame(const QImage &image);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onClientReadyRead();
    void onClientDisconnected();

private:
    void sendHttpHeader(QTcpSocket *socket);
    void sendJpegFrame(QTcpSocket *socket, const QByteArray &jpeg);

    QList<QTcpSocket *> clients;
    QElapsedTimer frameTimer;   // 帧率控制
    qint64 lastFrameMs = 0;
    const qint64 minFrameIntervalMs = 100;  // 最多 10fps，防止 CPU/带宽过高
};

#endif // STREAMSERVER_H
