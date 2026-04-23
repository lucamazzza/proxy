#ifndef APPCOMMCLIENT_H
#define APPCOMMCLIENT_H

#include <QObject>
#include "state.h"
#include "model.h"
#include <memory>
#include <QJsonObject>
#include <QJsonArray>
#include "appwritesdk.h"
#include "realtime.h"
#include "ratelimiter.h"
#include "recoverymanager.h"

namespace appcomm {

namespace client {

/*!
 * @brief High-level façade for the AppComm client.
 *
 * This class exposes a simplified interface for UI (QML/Qt Widgets)
 * to interact with the AppComm communication layer.
 *
 * It manages:
 * - authentication
 * - channel lifecycle
 * - message sending/receiving
 * - connection state exposure
 *
 * Internally it delegates logic to lower-level components.
 */
class AppcommClient : public QObject {
    Q_OBJECT

    /*!
     * @brief Current connection state.
     */
    Q_PROPERTY(ConnectionState connectionState
                   READ connectionState
                       NOTIFY connectionStateChanged
                           FINAL)

    /*!
     * @brief Human-readable connection state (for UI).
     */
    Q_PROPERTY(QString connectionStateText
                   READ connectionStateText
                       NOTIFY connectionStateChanged
                           FINAL)

    /*!
     * @brief Whether the client is authenticated.
     */
    Q_PROPERTY(bool authenticated
                   READ isAuthenticated
                       NOTIFY authenticationStateChanged
                           FINAL)
public:
    explicit AppcommClient(const model::AppCommConfig &config, QObject *parent = nullptr);

    explicit AppcommClient(
        const model::AppCommConfig &config,
        appwritesdk::IClientSdk *client,
        IRealtime *realtime,
        IRecoveryManager *recoveryManager,
        IRateLimiter *rateLimiter,
        QObject *parent = nullptr
    );

    ~AppcommClient() override;

    //Connection
    void connectToServer();
    void disconnectFromServer();

    ConnectionState connectionState() const;
    QString connectionStateText() const;

    //Authentication
    void createGuestSession();
    void createEmailSession(const QString &email, const QString &password);
    void logout();

    bool isAuthenticated() const;
    model::SessionInfo sessionInfo() const;

    //Channel
    void joinChannel(const QString &channelId);
    void leaveChannel();
    QString activeChannel() const;

    //Messaging
    void sendMessage(const QJsonObject &payload);

    void loadMembership();
    void loadChannelMessages(int limit = 50);
signals:
    //Authentication
    void authenticationStateChanged();

    //Connection
    void connectionStateChanged();

    //Channel
    void joinedChannel(const QString &channelId);
    void leftChannel(const QString &channelId);

    //Messaging
    void messageReceived(const model::Message &message);

    //Error
    void errorOccurred(const QString &errorMessage);
private slots:
    /*!
     * @brief Manages when messages are successfully recovered.
     *
     * @param messages Array of recovered messages in chronological order
     */
    void onMessagesRecovered(const QJsonArray &messages);

    /*!
     * @brief Manages when a recovery operation fails.
     *
     * @param errorCode HTTP error code or internal error code
     * @param errorMessage Description of the error
     */
    void onRecoveryError(int errorCode, const QString &errorMessage);

    /*!
     * @brief Manages when full resync completes successfully.
     *
     * Indicates that the client state has been fully realigned with server.
     *
     * @param messageCount Number of messages retrieved during resync
     */
    void onResyncCompleted(int messageCount);
    /*!
     * @brief Manages when an HTTP request completes successfully.
     *
     * @param data JSON response data from the server
     */
    void onRequestSuccess(const QJsonObject &data);

    /*!
     * @brief Manages when an HTTP request fails.
     *
     * @param code HTTP status code or error code
     * @param message Error message description
     */
    void onRequestError(int code, const QString &message);

    /*!
     * @brief Manages when WebSocket connection is successfully established.
     *
     * This signal is emitted after calling connect() once the WebSocket
     * handshake completes successfully. At this point, the client is ready
     * to receive real-time events.
     */
    void onConnected();

    /*!
     * @brief Manages when WebSocket connection is closed.
     *
     * This signal is emitted when the connection is closed, either by
     * calling disconnect() or if the server closes the connection.
     */
    void onDisconnected();

    /*!
     * @brief Manages when a real-time event is received from the server.
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
    void onEventReceived(const QJsonObject &event);

    /*!
     * @brief Manages when a WebSocket error occurs.
     *
     * This signal is emitted when the WebSocket encounters an error,
     * such as connection failure, network issues, or protocol errors.
     *
     * @param error Human-readable error description
     */
    void onErrorOccurred(const QString &error);
private:
    class Private; //pimpl
    std::unique_ptr<Private> d; //Data
    void setConnectionState(ConnectionState newState);
    void setupConnections();
};

} // namespace client

} // namespace appcomm

#endif // APPCOMMCLIENT_H