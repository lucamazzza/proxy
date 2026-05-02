/*!
 * @file realtime.cpp
 * @brief Implementation of Appwrite realtime websocket integration.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#include "realtime.h"

#include <QJsonDocument>
#include <QUrl>

using namespace appcomm;

Realtime::Realtime(const appwritesdk::ConnectionConfig &config, QObject *parent)
    : IRealtime(parent)
#ifndef APPCOMM_NO_WEBSOCKETS
    , m_webSocket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
#endif
    , m_config(config)
{
#ifndef APPCOMM_NO_WEBSOCKETS
    QObject::connect(m_webSocket, &QWebSocket::connected,
                     this, &Realtime::onConnected);

    QObject::connect(m_webSocket, &QWebSocket::disconnected,
                     this, &Realtime::onDisconnected);

    QObject::connect(m_webSocket, &QWebSocket::textMessageReceived,
                     this, &Realtime::onTextMessageReceived);

    QObject::connect(m_webSocket, &QWebSocket::errorOccurred,
                     this, &Realtime::onErrorOccurred);
#endif
}

void Realtime::connectToTopics(const QStringList &topics)
{
#ifdef APPCOMM_NO_WEBSOCKETS
    Q_UNUSED(topics);
    emit connected();
#else
    QString endpoint = m_config.endpoint;

    if (endpoint.startsWith("https://")) {
        endpoint.replace("https://", "wss://");
    } else if (endpoint.startsWith("http://")) {
        endpoint.replace("http://", "ws://");
    }

    if (endpoint.endsWith("/v1")) {
        endpoint.chop(3);
    }

    QString urlStr = QString("%1/v1/realtime?project=%2")
                         .arg(endpoint, m_config.projectId);

    for (const QString &topic : topics) {
        urlStr.append(QString("&channels[]=%1").arg(topic));
    }

    m_webSocket->open(QUrl(urlStr));
#endif
}

void Realtime::disconnectFromServer()
{
#ifdef APPCOMM_NO_WEBSOCKETS
    emit disconnected();
#else
    m_webSocket->close();
#endif
}

void Realtime::onConnected()
{
    emit connected();
}

void Realtime::onDisconnected()
{
    emit disconnected();
}

#ifndef APPCOMM_NO_WEBSOCKETS
void Realtime::onTextMessageReceived(const QString &msg)
{
    const QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());

    if (!doc.isObject()) {
        emit errorOccurred("Invalid realtime message: expected JSON object.");
        return;
    }

    emit eventReceived(doc.object());
}

void Realtime::onErrorOccurred(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    emit errorOccurred(m_webSocket->errorString());
}
#endif
