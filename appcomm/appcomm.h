/*!
 * @file appcomm.h
 * @brief Entry point for the fundamental functionalities of `appcomm`
 *
 * <Description...>
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#ifndef APPCOMM_H
#define APPCOMM_H

#include <QObject>

/*!
 * @brief AppComm library namespace
 *
 * Contains classes and types for managing communication with Appwrite
 * instances through the proxy, including connection state management
 * and channel operations.
 */
namespace appcomm {

/*!
 * @brief Client class for managing channel communication with Appwrite
 */
class ChannelClient : public QObject
{
    Q_OBJECT

public:
    /*!
     * @brief Enumerates the possible states of the proxy
     *
     * This can be used to inform the user about their connection
     * state with the `Appwrite` instance through the proxy and/or
     * prevent them from starting communication if the connection
     * is not established or functioning.
     */
    enum class ConnectionState {
        Disconnected = 0, ///< The proxy has no connection to Appwrite.
        Connecting,       ///< The proxy is establishing a connection.
        Connected,        ///< The proxy is connected to an instance.
        Reconnecting,     ///< The proxy is trying to reconnect.
        Error             ///< The proxy is experiencing some errors.
    };
    Q_ENUM(ConnectionState)

    explicit ChannelClient(QObject *parent = nullptr) : QObject(parent) {}
};

} // namespace appcomm

#endif // APPCOMM_H
