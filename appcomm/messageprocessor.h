#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include "state.h"
#include <QObject>

using namespace appcomm::client;

class MessageProcessor : public QObject {
    Q_OBJECT
public:
    explicit MessageProcessor(ClientState* clientstate);
private:
    ClientState* clientstate;
};

#endif // MESSAGEPROCESSOR_H
