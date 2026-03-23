/*!
 * @file recoverymanager.h
 * @brief Message recovery manager for handling missing messages and resync
 *
 * Provides functionality to recover missing messages from the server,
 * request specific messages, or perform full resynchronization.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#ifndef RECOVERYMANAGER_H
#define RECOVERYMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include "appwritesdk.h"
#include "recentmessagecache.h"

namespace appcomm {

/*!
 * @brief Manager for recovering missing messages and synchronizing client state.
 *
 * The RecoveryManager handles three types of recovery operations:
 * 1. requestFrom(messageId) - Recover all messages after a specific message ID
 * 2. request(messageId) - Recover a single specific message
 * 3. requestFullResync() - Perform complete resynchronization with server
 *
 * After successful recovery, the client state is realigned with the server.
 */
class RecoveryManager : public QObject {
    Q_OBJECT

public:
    /*!
     * @brief Constructs a RecoveryManager instance.
     *
     * @param client Appwrite client SDK for making requests
     * @param cache Message cache for storing recovered messages
     * @param config Appwrite connection configuration
     * @param parent Parent QObject for memory management
     */
    explicit RecoveryManager(appwritesdk::Client *client,
                           RecentMessageCache *cache,
                           const appwritesdk::ConnectionConfig &config,
                           QObject *parent = nullptr);

    /*!
     * @brief Requests all messages that came after a specific message ID.
     *
     * First checks the local cache. If messages are found in cache, emits
     * them immediately. Otherwise, queries the server for messages with
     * timestamp greater than the specified message.
     *
     * @param messageId Starting point (exclusive) - get all messages after this
     */
    void requestFrom(const QString &messageId);

    /*!
     * @brief Requests a single specific message by ID.
     *
     * First checks the local cache. If not found, queries the server
     * to retrieve the specific message document.
     *
     * @param messageId The specific message ID to retrieve
     */
    void request(const QString &messageId);

    /*!
     * @brief Requests a full resynchronization with the server.
     *
     * Clears the local cache and retrieves all available messages
     * from the server, effectively resetting the client state to
     * match the server state.
     */
    void requestFullResync();

signals:
    /*!
     * @brief Emitted when messages are successfully recovered.
     *
     * @param messages Array of recovered messages in chronological order
     */
    void messagesRecovered(const QJsonArray &messages);

    /*!
     * @brief Emitted when a recovery operation fails.
     *
     * @param errorCode HTTP error code or internal error code
     * @param errorMessage Description of the error
     */
    void recoveryError(int errorCode, const QString &errorMessage);

    /*!
     * @brief Emitted when full resync completes successfully.
     *
     * Indicates that the client state has been fully realigned with server.
     *
     * @param messageCount Number of messages retrieved during resync
     */
    void resyncCompleted(int messageCount);

private slots:
    /*!
     * @brief Handles successful response from Appwrite client.
     *
     * @param data JSON response data from server
     */
    void onRequestSuccess(const QJsonObject &data);

    /*!
     * @brief Handles error response from Appwrite client.
     *
     * @param code Error code
     * @param message Error message
     */
    void onRequestError(int code, const QString &message);

private:
    /*!
     * @brief Current operation type for handling async responses.
     */
    enum class OperationType {
        None,
        RequestFrom,
        RequestSingle,
        FullResync
    };

    /*!
     * @brief Processes recovered messages and updates cache.
     *
     * @param messages Array of message documents from server
     */
    void processRecoveredMessages(const QJsonArray &messages);

    appwritesdk::Client *m_client;           ///< Appwrite client for API calls
    RecentMessageCache *m_cache;             ///< Local message cache
    appwritesdk::ConnectionConfig m_config;  ///< Appwrite connection config
    OperationType m_currentOperation;        ///< Current operation being performed
    QString m_currentMessageId;              ///< MessageId for current operation
};

} // namespace appcomm

#endif // RECOVERYMANAGER_H
