#ifndef STATE_H
#define STATE_H

#include <QObject>

namespace appcomm {

namespace client {

struct ClientState {
    QString lastReceivedMessageId;
    QString activeChannelId;
    bool authenticated;
};

enum class ConnectionState {
    Disconnecting,
    Connecting,
    Connected,
    Reconnecting,
    Failed
};

}//namespace client

}//namespace appcomm

#endif // STATE_H
