/*!
 * @file realtime.h
 * @brief Realtime Wrapper for WebSockets.
 *
 * @copyright Copyright (c) 2026 SUPSI
 *
 */

#ifndef REALTIME_H
#define REALTIME_H

#include <QObject>
#include <QtWebSockets/QtWebSockets>

#include "appwritesdk.h"

using namespace appwritesdk;

namespace appcomm {

/*!
 * @brief Realtime Appwrite SDK for subscribing to events.
 *
 * Provides WebSocket-based communication to receive instant updates
 * for database changes, function executions, and other system events.
 */
class Realtime : public QObject {
    Q_OBJECT
public:

    /*!
     * @brief Realtime Appwrite SDK for subscribing to events.
     *
     * Provides WebSocket-based communication to receive instant updates
     * for database changes, function executions, and other system events.
     */
    explicit Realtime(const appwritesdk::ConnectionConfig &config, QObject *parent = nullptr);

    /*!
     * @brief Establishes the WebSocket connection with subscriptions.
     *
     * @param channels List of channels to subscribe to (e.g., "databases.A.collections.B.documents")
     */
    void connect(const QStringList &channels);

    /*!
     * @brief Closes the WebSocket connection.
     */
    void disconnect();

signals:

    /*!
     * @brief Emitted when the WebSocket connection is successfully established.
     */
    void connected();

    /*!
     * @brief Emitted when the WebSocket connection is closed.
     */
    void disconnected();

    /*!
     * @brief Emitted when a realtime event is received from the server.
     *
     * @param event JSON object containing event data and payload
     */
    void eventReceived(const QJsonObject &event);

    /*!
     * @brief Emitted when a realtime event is received from the server.
     *
     * @param event JSON object containing event data and payload
     */
    void errorOccourred(const QString &error);

private slots:

    /*!
     * @brief Handles connection to WebSocket.
     */
    void onConnected();

    /*!
     * @brief Handles disconnection from WebSocket.
     */
    void onDisconnected();

    /*!
     * @brief Handles reception of text messages through the WebSocket.
     *
     * @param msg The message received from the WebSocket.
     */
    void onTextMessageReceived(const QString &msg);

    /*!
     * @brief Handles errors occourred on the WebSocket's connection.
     *
     * @param error The SocketError occourred.
     */
    void onErrorOccurred(QAbstractSocket::SocketError error);

private:
    QWebSocket *m_webSocket;                ///< WebSocket instance for the transport
    appwritesdk::ConnectionConfig m_config; ///< Connection configuration to Appwrite
};

}

#endif // REALTIME_H
