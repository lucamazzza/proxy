#ifndef REALTIME_H
#define REALTIME_H

#include <QObject>
#include <QJsonObject>
#include <QStringList>
#include <QtWebSockets/QtWebSockets>

#include "appwritesdk.h"

namespace appcomm {

/*!
 * @brief Interface for real-time communication.
 *
 * Allows AppcommClient to depend on an abstraction instead of the concrete
 * Realtime class. This makes AppcommClient testable with fake realtime objects.
 */
class IRealtime : public QObject
{
    Q_OBJECT

public:
    explicit IRealtime(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~IRealtime() override = default;

    virtual void connectToTopics(const QStringList &topics) = 0;
    virtual void disconnectFromServer() = 0;

signals:
    void connected();
    void disconnected();
    void eventReceived(const QJsonObject &event);
    void errorOccurred(const QString &error);
};

/*!
 * @brief Real-time event subscription client using WebSockets.
 */
class Realtime : public IRealtime
{
    Q_OBJECT

public:
    explicit Realtime(const appwritesdk::ConnectionConfig &config,
                      QObject *parent = nullptr);

    void connectToTopics(const QStringList &topics) override;
    void disconnectFromServer() override;

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &msg);
    void onErrorOccurred(QAbstractSocket::SocketError error);

private:
    QWebSocket *m_webSocket;
    appwritesdk::ConnectionConfig m_config;
};

} // namespace appcomm

#endif // REALTIME_H
