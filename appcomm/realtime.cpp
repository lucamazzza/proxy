#include "realtime.h"

using namespace appcomm;

Realtime::Realtime(const appwritesdk::ConnectionConfig &config, QObject *parent)
    : QObject{parent}
    , m_config(config)
{
    m_webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    QObject::connect(m_webSocket, &QWebSocket::connected, this, &Realtime::onConnected);
    QObject::connect(m_webSocket, &QWebSocket::disconnected, this, &Realtime::onDisconnected);
    QObject::connect(m_webSocket, &QWebSocket::textMessageReceived, this, &Realtime::onTextMessageReceived);
    QObject::connect(m_webSocket, &QWebSocket::errorOccurred, this, &Realtime::onErrorOccurred);
}

void Realtime::connect(const QStringList &channels) {
    QString endpoint = m_config.endpoint;
    if (endpoint.startsWith("https://")) endpoint.replace("https://", "wss://");
    else if (endpoint.startsWith("http://")) endpoint.replace("http://", "ws://");
    if (endpoint.endsWith("/v1")) endpoint.chop(3);
    QString urlStr = QString("%1/v1/realtime?project=%2").arg(endpoint, m_config.projectId);
    for (const QString &channel : channels) urlStr.append(QString("&channels[]=%1").arg(channel));
    m_webSocket->open(QUrl(urlStr));
}

void Realtime::disconnect() {
    m_webSocket->close();
}

void Realtime::onConnected() {
    emit connected();
}

void Realtime::onDisconnected() {
    emit disconnected();
}

void Realtime::onTextMessageReceived(const QString &msg) {
    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
    if (doc.isObject()) emit eventReceived(doc.object());
}

void Realtime::onErrorOccurred(QAbstractSocket::SocketError error) {
    Q_UNUSED(error);
    emit errorOccurred(m_webSocket->errorString());
}
