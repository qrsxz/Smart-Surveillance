#include "StreamServer.h"

#include <QBuffer>
#include <QImageWriter>

StreamServer::StreamServer(QObject *parent) : QTcpServer(parent)
{
    frameTimer.start();
}

bool StreamServer::startServer(quint16 port)
{
    return listen(QHostAddress::Any, port);
}

void StreamServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, &QTcpSocket::readyRead, this, &StreamServer::onClientReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &StreamServer::onClientDisconnected);

    // 加入列表，等浏览器发来 GET 请求后再发画面
    clients.append(socket);
}

void StreamServer::onClientReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket)
        return;

    // 浏览器发来的 HTTP GET 请求，读一次即可
    socket->readAll();
    sendHttpHeader(socket);
    // 响应头已发，后续不再需要 readyRead
    disconnect(socket, &QTcpSocket::readyRead, this, &StreamServer::onClientReadyRead);
}

void StreamServer::onClientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket)
        return;
    clients.removeAll(socket);
    socket->deleteLater();
}

// 发送 HTTP 响应头：告诉浏览器这是持续更新的 MJPEG 流
void StreamServer::sendHttpHeader(QTcpSocket *socket)
{
    const char *header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: close\r\n"
        "\r\n";
    socket->write(header);
    socket->flush();
}

// 发送一帧 JPEG 数据
void StreamServer::sendJpegFrame(QTcpSocket *socket, const QByteArray &jpeg)
{
    QByteArray block;
    block += "--frame\r\n";
    block += "Content-Type: image/jpeg\r\n";
    block += "Content-Length: " + QByteArray::number(jpeg.size()) + "\r\n";
    block += "\r\n";
    block += jpeg;
    block += "\r\n";

    if (socket->write(block) == -1)
    {
        // 写入失败，客户端可能已断开
        socket->disconnectFromHost();
        clients.removeAll(socket);
        socket->deleteLater();
    }
    else
    {
        socket->flush();
    }
}

void StreamServer::broadcastFrame(const QImage &image)
{
    if (clients.isEmpty())
        return;  // 没人看就不编码，节省 CPU

    // 帧率控制：限制最小间隔，避免高帧率拖累 CPU
    qint64 now = frameTimer.elapsed();
    if (now - lastFrameMs < minFrameIntervalMs)
        return;
    lastFrameMs = now;

    // 缩放到合适宽度（640px 足够手机看，带宽可控）
    QImage frame = image;
    if (frame.width() > 640)
        frame = frame.scaledToWidth(640, Qt::SmoothTransformation);

    // 编码为 JPEG
    QByteArray jpeg;
    QBuffer buffer(&jpeg);
    buffer.open(QIODevice::WriteOnly);

    QImageWriter writer(&buffer, "jpeg");
    writer.setQuality(70);
    if (!writer.write(frame))
        return;
    buffer.close();

    // 广播给所有客户端
    for (QTcpSocket *socket : clients)
        sendJpegFrame(socket, jpeg);
}
