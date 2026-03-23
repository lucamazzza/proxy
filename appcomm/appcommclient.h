#ifndef APPCOMMCLIENT_H
#define APPCOMMCLIENT_H

#include <QObject>

namespace appcomm {

namespace client {

class AppcommClient : public QObject {
    Q_OBJECT
public:
    explicit AppcommClient(QObject *parent = nullptr);
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
signals:
};

} // namespace client

} // namespace appcomm

#endif // APPCOMMCLIENT_H
