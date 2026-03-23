/*!
 * @file realtime.h
 * @brief Real-time WebSocket wrapper for Appwrite event subscriptions.
 *
 * Provides WebSocket-based communication to receive instant updates
 * for database changes, authentication events, and other real-time
 * system events from Appwrite backend.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#ifndef REALTIME_H
#define REALTIME_H

#include <QObject>
#include <QtWebSockets/QtWebSockets>

#include "appwritesdk.h"

using namespace appwritesdk;

namespace appcomm {

/*!
 * @brief Real-time event subscription client using WebSockets.
 *
 * The Realtime class provides WebSocket-based communication with the Appwrite
 * backend to receive instant notifications about database changes, document
 * updates, authentication events, and other system events.
 *
 * Features:
 * - Automatic WebSocket connection management
 * - Channel-based subscriptions (databases, collections, documents)
 * - Event parsing and distribution via Qt signals
 * - Error handling and reconnection support
 *
 * Usage:
 * @code
 * Realtime realtime(config);
 * connect(&realtime, &Realtime::eventReceived, [](const QJsonObject &event) {
 *     qDebug() << "Event:" << event;
 * });
 * realtime.connect({"databases.mydb.collections.messages.documents"});
 * @endcode
 *
 * @see AppwriteSDK::ConnectionConfig for configuration details
 */
class Realtime : public QObject {
    Q_OBJECT
public:

    /*!
     * @brief Constructs a Realtime client instance.
     *
     * Initializes the WebSocket client with the provided Appwrite configuration.
     * The WebSocket is not connected until connect() is called.
     *
     * @param config Connection configuration (endpoint, projectId, etc.)
     * @param parent Parent QObject for memory management
     *
     * @see connect()
     */
    explicit Realtime(const appwritesdk::ConnectionConfig &config, QObject *parent = nullptr);

    /*!
     * @brief Establishes WebSocket connection and subscribes to channels.
     *
     * Opens a WebSocket connection to the Appwrite real-time endpoint and
     * subscribes to the specified channels. The connection URL is automatically
     * constructed from the configuration endpoint, replacing http(s):// with
     * ws(s)://.
     *
     * Channel format examples:
     * - "databases.{databaseId}.collections.{collectionId}.documents"
     * - "databases.{databaseId}.collections.{collectionId}.documents.{documentId}"
     * - "account" (for current user events)
     *
     * @param channels List of channel patterns to subscribe to
     *
     * @note Multiple channels can be subscribed simultaneously
     * @note Emits connected() signal when connection is established
     * @note Emits errorOccurred() if connection fails
     *
     * @see connected()
     * @see disconnect()
     */
    void connect(const QStringList &channels);

    /*!
     * @brief Closes the WebSocket connection.
     *
     * Gracefully closes the WebSocket connection to the Appwrite server.
     * Emits disconnected() signal when the connection is fully closed.
     *
     * @see disconnected()
     */
    void disconnect();

signals:

    /*!
     * @brief Emitted when WebSocket connection is successfully established.
     *
     * This signal is emitted after calling connect() once the WebSocket
     * handshake completes successfully. At this point, the client is ready
     * to receive real-time events.
     */
    void connected();

    /*!
     * @brief Emitted when WebSocket connection is closed.
     *
     * This signal is emitted when the connection is closed, either by
     * calling disconnect() or if the server closes the connection.
     */
    void disconnected();

    /*!
     * @brief Emitted when a real-time event is received from the server.
     *
     * Events contain information about changes in the Appwrite backend,
     * such as document creations, updates, deletions, or authentication events.
     *
     * Event structure:
     * @code
     * {
     *   "events": ["databases.*.collections.*.documents.*"],
     *   "channels": ["databases.db1.collections.messages.documents"],
     *   "timestamp": 1234567890,
     *   "payload": { ... document data ... }
     * }
     * @endcode
     *
     * @param event JSON object containing event metadata and payload
     *
     * @see Appwrite Real-time documentation for event structure details
     */
    void eventReceived(const QJsonObject &event);

    /*!
     * @brief Emitted when a WebSocket error occurs.
     *
     * This signal is emitted when the WebSocket encounters an error,
     * such as connection failure, network issues, or protocol errors.
     *
     * @param error Human-readable error description
     */
    void errorOccurred(const QString &error);

private slots:

    /*!
     * @brief Internal slot handling WebSocket connection establishment.
     *
     * Called automatically when the WebSocket successfully connects.
     * Emits the connected() signal to notify external listeners.
     */
    void onConnected();

    /*!
     * @brief Internal slot handling WebSocket disconnection.
     *
     * Called automatically when the WebSocket connection closes.
     * Emits the disconnected() signal to notify external listeners.
     */
    void onDisconnected();

    /*!
     * @brief Internal slot handling incoming text messages.
     *
     * Parses incoming WebSocket text messages as JSON and emits
     * the eventReceived() signal with the parsed event object.
     *
     * @param msg Raw text message received from WebSocket
     */
    void onTextMessageReceived(const QString &msg);

    /*!
     * @brief Internal slot handling WebSocket errors.
     *
     * Called when the WebSocket encounters an error. Formats
     * the error message and emits the errorOccurred() signal.
     *
     * @param error Socket error type (connection, protocol, etc.)
     */
    void onErrorOccurred(QAbstractSocket::SocketError error);

private:
    QWebSocket *m_webSocket;                ///< WebSocket instance for real-time transport
    appwritesdk::ConnectionConfig m_config; ///< Appwrite connection configuration
};

}

#endif // REALTIME_H
