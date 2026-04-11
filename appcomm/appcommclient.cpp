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
    std::unique_ptr<MessageProcessor> m_messageProcessor;

    // Componenti concreti posseduti da AppcommClient
    std::unique_ptr<appwritesdk::Client> m_ownedClient;
    std::unique_ptr<RecoveryManager> m_ownedRecoveryManager;
    std::unique_ptr<RateLimiter> m_ownedRateLimiter;
    std::unique_ptr<appcomm::Realtime> m_ownedRealtime;

    // Dipendenze effettivamente usate dal client
    appwritesdk::IClientSdk *m_clientSdk = nullptr;
    IRecoveryManager *m_recoveryManager = nullptr;
    IRateLimiter *m_rateLimiter = nullptr;
    appcomm::IRealtime *m_realtime = nullptr;

    // Puntatori QObject per fare connect ai segnali
    QObject *m_clientObject = nullptr;
    QObject *m_recoveryObject = nullptr;
    QObject *m_realtimeObject = nullptr;

    explicit Private(const appwritesdk::ConnectionConfig &cfg)
        : m_config(cfg),
        m_clientState(),
        m_connectionState(ConnectionState::Disconnected),
        m_sessionInfo(),
        m_recentMessageCache()
    {
        m_ownedClient = std::make_unique<appwritesdk::Client>();
        m_ownedRecoveryManager = std::make_unique<RecoveryManager>(
            m_ownedClient.get(),
            &m_recentMessageCache,
            m_config
            );
        m_ownedRateLimiter = std::make_unique<RateLimiter>(10, 1);
        m_ownedRealtime = std::make_unique<appcomm::Realtime>(m_config);

        m_messageProcessor = std::make_unique<MessageProcessor>(
            &m_clientState,
            m_ownedRecoveryManager.get()
            );

        m_clientSdk = m_ownedClient.get();
        m_recoveryManager = m_ownedRecoveryManager.get();
        m_rateLimiter = m_ownedRateLimiter.get();
        m_realtime = m_ownedRealtime.get();

        m_clientObject = m_ownedClient.get();
        m_recoveryObject = m_ownedRecoveryManager.get();
        m_realtimeObject = m_ownedRealtime.get();
    }

    Private(const appwritesdk::ConnectionConfig &cfg,
            appwritesdk::IClientSdk *clientSdk,
            appcomm::IRealtime *realtime,
            IRecoveryManager *recoveryManager,
            IRateLimiter *rateLimiter)
        : m_config(cfg),
        m_clientState(),
        m_connectionState(ConnectionState::Disconnected),
        m_sessionInfo(),
        m_recentMessageCache(),
        m_clientSdk(clientSdk),
        m_recoveryManager(recoveryManager),
        m_rateLimiter(rateLimiter),
        m_realtime(realtime)
    {
        m_messageProcessor = std::make_unique<MessageProcessor>(
            &m_clientState,
            m_recoveryManager
            );

        m_clientObject = dynamic_cast<QObject *>(clientSdk);
        m_recoveryObject = dynamic_cast<QObject *>(recoveryManager);
        m_realtimeObject = dynamic_cast<QObject *>(realtime);

        Q_ASSERT(m_clientObject);
        Q_ASSERT(m_recoveryObject);
        Q_ASSERT(m_realtimeObject);
    }
};

appcomm::client::AppcommClient::AppcommClient(const appwritesdk::ConnectionConfig &config,
                             QObject *parent)
    : QObject(parent),
    d(std::make_unique<Private>(config))

{
    connect(d->m_recoveryObject, SIGNAL(messagesRecovered(QJsonArray)),
            this, SLOT(onMessagesRecovered(QJsonArray)));

    connect(d->m_recoveryObject, SIGNAL(recoveryError(int,QString)),
            this, SLOT(onRecoveryError(int,QString)));

    connect(d->m_recoveryObject, SIGNAL(resyncCompleted(int)),
            this, SLOT(onResyncCompleted(int)));

    connect(d->m_clientObject, SIGNAL(requestSuccess(appwritesdk::RequestType,QJsonObject)),
            this, SLOT(onRequestSuccess(appwritesdk::RequestType,QJsonObject)));

    connect(d->m_clientObject, SIGNAL(requestError(appwritesdk::RequestType,int,QString)),
            this, SLOT(onRequestError(appwritesdk::RequestType,int,QString)));

    connect(d->m_realtimeObject, SIGNAL(connected()),
            this, SLOT(onConnected()));

    connect(d->m_realtimeObject, SIGNAL(disconnected()),
            this, SLOT(onDisconnected()));

    connect(d->m_realtimeObject, SIGNAL(eventReceived(QJsonObject)),
            this, SLOT(onEventReceived(QJsonObject)));

    connect(d->m_realtimeObject, SIGNAL(errorOccurred(QString)),
            this, SLOT(onErrorOccurred(QString)));
}

appcomm::client::AppcommClient::AppcommClient(
    const appwritesdk::ConnectionConfig &config,
    appwritesdk::IClientSdk *clientSdk,
    appcomm::IRealtime *realtime,
    IRecoveryManager *recoveryManager,
    IRateLimiter *rateLimiter,
    QObject *parent)
    : QObject(parent),
    d(std::make_unique<Private>(config,
                                clientSdk,
                                realtime,
                                recoveryManager,
                                rateLimiter))
{
    connect(d->m_recoveryObject, SIGNAL(messagesRecovered(QJsonArray)),
            this, SLOT(onMessagesRecovered(QJsonArray)));

    connect(d->m_recoveryObject, SIGNAL(recoveryError(int,QString)),
            this, SLOT(onRecoveryError(int,QString)));

    connect(d->m_recoveryObject, SIGNAL(resyncCompleted(int)),
            this, SLOT(onResyncCompleted(int)));

    connect(d->m_clientObject, SIGNAL(requestSuccess(appwritesdk::RequestType,QJsonObject)),
            this, SLOT(onRequestSuccess(appwritesdk::RequestType,QJsonObject)));

    connect(d->m_clientObject, SIGNAL(requestError(appwritesdk::RequestType,int,QString)),
            this, SLOT(onRequestError(appwritesdk::RequestType,int,QString)));

    connect(d->m_realtimeObject, SIGNAL(connected()),
            this, SLOT(onConnected()));

    connect(d->m_realtimeObject, SIGNAL(disconnected()),
            this, SLOT(onDisconnected()));

    connect(d->m_realtimeObject, SIGNAL(eventReceived(QJsonObject)),
            this, SLOT(onEventReceived(QJsonObject)));

    connect(d->m_realtimeObject, SIGNAL(errorOccurred(QString)),
            this, SLOT(onErrorOccurred(QString)));
}

AppcommClient::~AppcommClient() = default;

//Connection
void AppcommClient::connectToServer()
{
    if (!d->m_clientState.authenticated) {
        return;
    }

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

    d->m_realtime->connectToChannels({channel});
}

void AppcommClient::disconnectFromServer()
{
    if (d->m_connectionState == ConnectionState::Disconnected ||
        d->m_connectionState == ConnectionState::Disconnecting) {
        return;
    }

    setConnectionState(ConnectionState::Disconnecting);
    d->m_realtime->disconnectFromServer();
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
    d->m_clientSdk->createAnonymousSession(d->m_config);
}

void AppcommClient::createEmailSession(const QString &email, const QString &password)
{
    d->m_clientSdk->createEmailSession(d->m_config, email, password);
}

void AppcommClient::logout()
{
    const bool wasAuthenticated = d->m_clientState.authenticated;

    if (!d->m_sessionInfo.sessionId.trimmed().isEmpty()) {
        d->m_clientSdk->deleteSession(d->m_config, d->m_sessionInfo.sessionId);
    }

    d->m_sessionInfo = {};
    d->m_clientState.authenticated = false;

    if (wasAuthenticated) {
        emit authenticationStateChanged();
    }
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
    d->m_messageProcessor->reset(trimmedChannelId);

    emit activeChannelChanged(trimmedChannelId);

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
    d->m_messageProcessor->reset(QString());

    emit leftChannel(oldChannelId);

    if (d->m_connectionState != ConnectionState::Disconnected &&
        d->m_connectionState != ConnectionState::Disconnecting) {
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

    if (!d->m_rateLimiter->allowRequest()) {
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

    d->m_clientSdk->createDocument(d->m_config, msg.toJson());
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

        handleIncomingMessage(message);
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
void AppcommClient::onRequestSuccess(appwritesdk::RequestType type,
                                     const QJsonObject &data)
{
    switch (type) {
    case appwritesdk::RequestType::CreateAnonymousSession:
    case appwritesdk::RequestType::CreateEmailSession:
        if (!d->m_clientState.authenticated &&
            data.contains("$id") &&
            data.value("$id").isString()) {
            d->m_sessionInfo.sessionId = data.value("$id").toString();
            if (data.contains("userId") && data.value("userId").isString()) {
                d->m_sessionInfo.userId = data.value("userId").toString();
            } else {
                d->m_sessionInfo.userId.clear();
            }
            d->m_clientState.authenticated = true;
            emit authenticationStateChanged();
        }
        break;

    default:
        break;
    }
}

void AppcommClient::onRequestError(appwritesdk::RequestType type,
                                   int code,
                                   const QString &message)
{
    Q_UNUSED(type);
    Q_UNUSED(code);
    emit errorOccurred(message);
}

//Realtime slots
void AppcommClient::onConnected()
{
    setConnectionState(ConnectionState::Connected);

    if (!d->m_clientState.activeChannelId.isEmpty()) {
        emit channelReady(d->m_clientState.activeChannelId);
    }
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

    handleIncomingMessage(message);
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

void AppcommClient::handleIncomingMessage(const model::Message &message)
{
    const std::optional<appcomm::model::Message> processed =
        d->m_messageProcessor->processIncoming(message);

    if (!processed.has_value()) {
        return;
    }

    d->m_recentMessageCache.addMessage(*processed);
    emit messageReceived(*processed);
}
