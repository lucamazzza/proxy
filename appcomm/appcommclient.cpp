#include "appcommclient.h"

#include "recentmessagecache.h"
#include "recoverymanager.h"
#include "messageprocessor.h"
#include "ratelimiter.h"
#include "realtime.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QUuid>

#include <optional>

using namespace appcomm::client;

class AppcommClient::Private {
public:

    appwritesdk::ConnectionConfig m_config;

    ClientState m_clientState;
    ConnectionState m_connectionState = ConnectionState::Disconnected;
    model::SessionInfo m_sessionInfo;

    RecentMessageCache m_recentMessageCache;
    appwritesdk::Client m_client;
    RecoveryManager m_recoveryManager;
    RateLimiter m_rateLimiter;
    MessageProcessor m_messageProcessor;
    Realtime m_realtime;

    Private(const appwritesdk::ConnectionConfig &cfg):
        m_config(cfg),
        m_clientState(),
        m_connectionState(ConnectionState::Disconnected),
        m_sessionInfo(),
        m_recentMessageCache(),
        m_client(nullptr),
        m_recoveryManager(&m_client, &m_recentMessageCache, m_config),
        m_rateLimiter(10, 1),
        m_messageProcessor(&m_clientState, &m_recoveryManager),
        m_realtime(m_config)
    {}
};

appcomm::client::AppcommClient::AppcommClient(const appwritesdk::ConnectionConfig &config,
                             QObject *parent)
    : QObject(parent),
    d(std::make_unique<Private>(config))

{
    connect(&d->m_recoveryManager, &RecoveryManager::messagesRecovered,
            this, &AppcommClient::onMessagesRecovered);

    connect(&d->m_recoveryManager, &RecoveryManager::recoveryError,
            this, &AppcommClient::onRecoveryError);

    connect(&d->m_recoveryManager, &RecoveryManager::resyncCompleted,
            this, &AppcommClient::onResyncCompleted);

    connect(&d->m_client, &appwritesdk::Client::requestSuccess,
            this, &AppcommClient::onRequestSuccess);

    connect(&d->m_client, &appwritesdk::Client::requestError,
            this, &AppcommClient::onRequestError);

    connect(&d->m_realtime, &Realtime::connected,
            this, &AppcommClient::onConnected);

    connect(&d->m_realtime, &Realtime::disconnected,
            this, &AppcommClient::onDisconnected);

    connect(&d->m_realtime, &Realtime::eventReceived,
            this, &AppcommClient::onEventReceived);

    connect(&d->m_realtime, &Realtime::errorOccurred,
            this, &AppcommClient::onErrorOccurred);
}

AppcommClient::~AppcommClient() = default;

//Connection
void AppcommClient::connectToServer()
{
    if (d->m_connectionState == ConnectionState::Connected ||
        d->m_connectionState == ConnectionState::Connecting) {
        return;
    }

    if (d->m_clientState.activeChannelId.trimmed().isEmpty()) {
        return;
    }

    setConnectionState(ConnectionState::Connecting);

    const QString channel =
        QString("databases.%1.collections.%2.documents")
            .arg(d->m_config.dbId, d->m_config.collectionId);

    d->m_realtime.connect({channel});
}

void AppcommClient::disconnectFromServer()
{
    if (d->m_connectionState == ConnectionState::Disconnected ||
        d->m_connectionState == ConnectionState::Disconnecting) {
        return;
    }

    setConnectionState(ConnectionState::Disconnecting);
    d->m_realtime.disconnect();
}

ConnectionState AppcommClient::connectionState() const
{
    return d->m_connectionState;
}

QString AppcommClient::connectionStateText() const
{
    switch (d->m_connectionState) {
    case ConnectionState::Disconnected:
        return "Disconnected";
    case ConnectionState::Disconnecting:
        return "Disconnecting";
    case ConnectionState::Connected:
        return "Connected";
    case ConnectionState::Connecting:
        return "Connecting";
    case ConnectionState::Reconnecting:
        return "Reconnecting";
    case ConnectionState::Failed:
        return "Failed";
    }

    return "Unknown";
}

//Authentication
void AppcommClient::createGuestSession()
{
    d->m_client.createAnonymousSession(d->m_config);
}

void AppcommClient::createEmailSession(const QString &email, const QString &password)
{
    d->m_client.createEmailSession(d->m_config, email, password);
}

void AppcommClient::logout()
{
    if (!d->m_sessionInfo.sessionId.trimmed().isEmpty()) {
        d->m_client.deleteSession(d->m_config, d->m_sessionInfo.sessionId);
    }

    d->m_sessionInfo = {};
    d->m_clientState.authenticated = false;
    emit authenticationStateChanged();
}

bool AppcommClient::isAuthenticated() const
{
    return d->m_clientState.authenticated;
}

appcomm::model::SessionInfo AppcommClient::sessionInfo() const
{
    return d->m_sessionInfo;
}

//Channel
void AppcommClient::joinChannel(const QString &channelId)
{
    const QString trimmedChannelId = channelId.trimmed();
    if (trimmedChannelId.isEmpty()) {
        emit errorOccurred("Channel ID cannot be empty.");
        return;
    }

    d->m_clientState.activeChannelId = trimmedChannelId;
    d->m_messageProcessor.reset(trimmedChannelId);

    emit joinedChannel(trimmedChannelId);

    if (d->m_connectionState == ConnectionState::Disconnected) {
        connectToServer();
    }
}

void AppcommClient::leaveChannel()
{
    const QString oldChannelId = d->m_clientState.activeChannelId;
    if (oldChannelId.isEmpty()) {
        return;
    }

    d->m_clientState.activeChannelId.clear();
    d->m_messageProcessor.reset(QString());

    emit leftChannel(oldChannelId);

    if (d->m_connectionState == ConnectionState::Connected) {
        disconnectFromServer();
    }
}

QString AppcommClient::activeChannel() const
{
    return d->m_clientState.activeChannelId;
}

//Messaging
void AppcommClient::sendMessage(const QJsonObject &payload)
{
    if (!d->m_clientState.authenticated) {
        emit errorOccurred("Cannot send message: client is not authenticated.");
        return;
    }

    if (d->m_clientState.activeChannelId.trimmed().isEmpty()) {
        emit errorOccurred("Cannot send message: no active channel.");
        return;
    }

    if (!d->m_rateLimiter.allowRequest()) {
        emit errorOccurred("Rate limit exceeded.");
        return;
    }

    appcomm::model::Message msg;
    msg.channelId = d->m_clientState.activeChannelId;
    msg.senderId = d->m_sessionInfo.userId;
    msg.messageId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    msg.sequenceNumber = -1;
    msg.timestamp = QDateTime::currentDateTimeUtc();
    msg.payload = payload;
    msg.isEcho = false;

    d->m_client.createDocument(d->m_config, msg.toJson());
}

//Recovery slots
void AppcommClient::onMessagesRecovered(const QJsonArray &messages)
{
    for (const QJsonValue &value : messages) {
        if (!value.isObject()) {
            continue;
        }

        const appcomm::model::Message message =
            appcomm::model::Message::fromJson(value.toObject());

        const std::optional<appcomm::model::Message> processed =
            d->m_messageProcessor.processIncoming(message);

        if (!processed.has_value()) {
            continue;
        }

        d->m_recentMessageCache.addMessage(*processed);
        emit messageReceived(*processed);
    }
}

void AppcommClient::onRecoveryError(int errorCode, const QString &errorMessage)
{
    Q_UNUSED(errorCode);
    emit errorOccurred(errorMessage);
}

void AppcommClient::onResyncCompleted(int messageCount)
{
    Q_UNUSED(messageCount);
}

//HTTP slots
void AppcommClient::onRequestSuccess(const QJsonObject &data)
{
    if (data.contains("$id") && data.value("$id").isString()) {
        if (!d->m_clientState.authenticated) {
            d->m_sessionInfo.sessionId = data.value("$id").toString();
            d->m_sessionInfo.userId = data.value("userId").toString();
            d->m_clientState.authenticated = true;
            emit authenticationStateChanged();
            return;
        }
    }
}

void AppcommClient::onRequestError(int code, const QString &message)
{
    Q_UNUSED(code);
    emit errorOccurred(message);
}

//Realtime slots
void AppcommClient::onConnected()
{
    setConnectionState(ConnectionState::Connected);
}

void AppcommClient::onDisconnected()
{
    setConnectionState(ConnectionState::Disconnected);
}

void AppcommClient::onEventReceived(const QJsonObject &event)
{
    const QJsonValue payloadValue = event.value("payload");
    if (!payloadValue.isObject()) {
        return;
    }

    const appcomm::model::Message message =
        appcomm::model::Message::fromJson(payloadValue.toObject());

    const std::optional<appcomm::model::Message> processed =
        d->m_messageProcessor.processIncoming(message);

    if (!processed.has_value()) {
        return;
    }

    d->m_recentMessageCache.addMessage(*processed);
    emit messageReceived(*processed);
}

void AppcommClient::onErrorOccurred(const QString &error)
{
    setConnectionState(ConnectionState::Failed);
    emit errorOccurred(error);
}

//Internal
void AppcommClient::setConnectionState(ConnectionState newState)
{
    if (d->m_connectionState == newState) {
        return;
    }

    d->m_connectionState = newState;
    emit connectionStateChanged();
}
