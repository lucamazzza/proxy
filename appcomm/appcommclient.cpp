#include "appcommclient.h"

using namespace appcomm::client;

AppcommClient::AppcommClient(QObject *parent)
    : QObject{parent}
{}

ConnectionState AppcommClient::connectionState() const
{
    return m_connectionState;
}
