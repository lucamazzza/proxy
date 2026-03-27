/*!
 * @file state.h
 * @brief Client state and connection state definitions for the AppComm client module.
 *
 * Defines data structures and enumerations used to represent the runtime
 * state of the client, including connection lifecycle and session-related data.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#ifndef STATE_H
#define STATE_H

#include <QObject>
#include <QString>

/*!
 * @brief Namespace for AppComm client-side components.
 */
namespace appcomm {

/*!
 * @brief Namespace containing client-specific state definitions.
 */
namespace client {

/*!
 * @brief Represents the current state of the client session.
 *
 * Stores information about the active communication context and authentication status.
 * This structure is typically used to track the client's runtime state and synchronize
 * it with UI or higher-level components.
 */
struct ClientState {
    qint64 lastReceivedSequence = -1; ///< Number sequence of the last message received
    QString lastReceivedMessageId;    ///< ID of the last message received (used for ordering or recovery)
    QString activeChannelId;          ///< Identifier of the currently active channel
    bool authenticated = false;       ///< Indicates whether the client is authenticated
};

/*!
 * @brief Enumeration of possible connection states.
 *
 * Represents the lifecycle of the client's connection to the communication backend.
 * Used to manage UI state, reconnection logic, and network behavior.
 */
enum class ConnectionState {
    Disconnected = 0, ///< No active connection
    Disconnecting,    ///< Connection is being closed
    Connected,        ///< Connection is established and active
    Connecting,       ///< Connection attempt in progress
    Reconnecting,     ///< Attempting to restore a lost connection
    Failed            ///< Connection attempt failed
};

} // namespace client

} // namespace appcomm

#endif // STATE_H
