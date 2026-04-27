/*!
 * @file appcommclient.cpp
 * @brief Implementation of the high-level AppComm client facade.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#include "appcommclient.h"

#include "recentmessagecache.h"
#include "recoverymanager.h"
#include "messageprocessor.h"
#include "ratelimiter.h"
#include "realtime.h"
#include "appwritesdk.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QUuid>
#include <QNetworkAccessManager>

#include <optional>
#include <utility>

using namespace appcomm::client;

namespace {

/*!
 * @brief Checks whether a JSON response contains session information.
 * @param data Response payload.
 * @return True when the payload contains non-empty `$id` and `userId` fields.
 */
bool isSessionResponse(const QJsonObject &data) {
    const QJsonValue sessionIdValue = data.value("$id");
    const QJsonValue userIdValue = data.value("userId");

    return sessionIdValue.isString()
           && userIdValue.isString()
           && !sessionIdValue.toString().trimmed().isEmpty()
           && !userIdValue.toString().trimmed().isEmpty();
}

/*!
 * @brief Extracts a realtime event payload across supported envelope shapes.
 * @param event Raw realtime event object.
 * @return Message payload object, or an empty object when unavailable.
 */
QJsonObject extractRealtimePayload(const QJsonObject &event) {
    QJsonObject envelope = event;

    const QJsonValue dataValue = event.value("data");
    if (dataValue.isObject()) {
        envelope = dataValue.toObject();
    }

    QJsonValue payloadValue = envelope.value("payload");
    if (!payloadValue.isObject()) {
        payloadValue = event.value("payload");
    }

    if (!payloadValue.isObject()) {
        return {};
    }

    QJsonObject payload = payloadValue.toObject();

    const QJsonValue nestedDataValue = payload.value("data");
    if (nestedDataValue.isObject()) {
        payload = nestedDataValue.toObject();
    }

    return payload;
}

appwritesdk::ConnectionConfig makeBaseConfig(const appcomm::model::AppCommConfig &cfg)
{
    appwritesdk::ConnectionConfig baseConfig;
    baseConfig.endpoint = cfg.endpoint;
    baseConfig.projectId = cfg.projectId;
    baseConfig.apiKey = QString();
    baseConfig.dbId = cfg.databaseId;
    baseConfig.collectionId = cfg.messagesCollectionId;
    return baseConfig;
}

} // namespace

class AppcommClient::Private {
public:
    enum class PendingRequest {
        None,
        Login,
        LoadMembership,
        LoadMessages
    };

    model::AppCommConfig m_appConfig;
    appwritesdk::ConnectionConfig m_baseConfig;

    ClientState m_clientState;
    ConnectionState m_connectionState = ConnectionState::Disconnected;
    model::SessionInfo m_sessionInfo;

    RecentMessageCache m_recentMessageCache;
    QNetworkAccessManager m_networkManager;

    std::unique_ptr<appwritesdk::IClientSdk> m_client;
    std::unique_ptr<IRecoveryManager> m_recoveryManager;
    std::unique_ptr<IRateLimiter> m_rateLimiter;
    std::unique_ptr<IRealtime> m_realtime;

    std::unique_ptr<MessageProcessor> m_messageProcessor;
    PendingRequest m_pendingRequest = PendingRequest::None;

    explicit Private(const model::AppCommConfig &cfg)
        : m_appConfig(cfg)
        , m_baseConfig(makeBaseConfig(cfg))
        , m_clientState()
        , m_connectionState(ConnectionState::Disconnected)
        , m_sessionInfo()
        , m_recentMessageCache()
        , m_networkManager()
    {
        auto *client = new appwritesdk::Client(&m_networkManager);

        m_client = std::unique_ptr<appwritesdk::IClientSdk>(client);
        m_recoveryManager = std::unique_ptr<IRecoveryManager>(
            new RecoveryManager(client, &m_recentMessageCache, m_baseConfig)
            );
        m_rateLimiter = std::make_unique<RateLimiter>(10, 1);
        m_realtime = std::make_unique<Realtime>(m_baseConfig);
        m_messageProcessor = std::make_unique<MessageProcessor>(
            &m_clientState,
            m_recoveryManager.get()
            );
    }

    Private(const model::AppCommConfig &cfg,
            AppcommClient::Dependencies dependencies)
        : m_appConfig(cfg)
        , m_baseConfig(makeBaseConfig(cfg))
        , m_clientState()
        , m_connectionState(ConnectionState::Disconnected)
        , m_sessionInfo()
        , m_recentMessageCache()
        , m_networkManager()
        , m_client(std::move(dependencies.client))
        , m_recoveryManager(std::move(dependencies.recoveryManager))
        , m_rateLimiter(std::move(dependencies.rateLimiter))
        , m_realtime(std::move(dependencies.realtime))
    {
        m_messageProcessor = std::make_unique<MessageProcessor>(
            &m_clientState,
            m_recoveryManager.get()
            );
    }

    appwritesdk::ConnectionConfig configForCollection(const QString &collectionId) const
    {
        appwritesdk::ConnectionConfig config = m_baseConfig;
        config.collectionId = collectionId;
        return config;
    }
};

AppcommClient::AppcommClient(const model::AppCommConfig &config, QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Private>(config))
{
    setupConnections();
}

AppcommClient::AppcommClient(const model::AppCommConfig &config,
                             Dependencies dependencies,
                             QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Private>(
          config,
          std::move(dependencies)
          ))
{
    setupConnections();
}

AppcommClient::~AppcommClient() = default;

void AppcommClient::setupConnections()
{
    auto *clientObject = dynamic_cast<QObject *>(d->m_client.get());
    if (clientObject != nullptr) {
        connect(clientObject, SIGNAL(requestSuccess(QJsonObject)),
                this, SLOT(onRequestSuccess(QJsonObject)));

        connect(clientObject, SIGNAL(requestError(int,QString)),
                this, SLOT(onRequestError(int,QString)));
    }

    if (d->m_realtime != nullptr) {
        connect(d->m_realtime.get(), &IRealtime::connected,
                this, &AppcommClient::onConnected);

        connect(d->m_realtime.get(), &IRealtime::disconnected,
                this, &AppcommClient::onDisconnected);

        connect(d->m_realtime.get(), &IRealtime::eventReceived,
                this, &AppcommClient::onEventReceived);

        connect(d->m_realtime.get(), &IRealtime::errorOccurred,
                this, &AppcommClient::onErrorOccurred);
    }

    auto *recoveryObject = dynamic_cast<QObject *>(d->m_recoveryManager.get());
    if (recoveryObject != nullptr) {
        connect(recoveryObject, SIGNAL(messagesRecovered(QJsonArray)),
                this, SLOT(onMessagesRecovered(QJsonArray)));

        connect(recoveryObject, SIGNAL(recoveryError(int,QString)),
                this, SLOT(onRecoveryError(int,QString)));

        connect(recoveryObject, SIGNAL(resyncCompleted(int)),
                this, SLOT(onResyncCompleted(int)));
    }
}

// Connection

void AppcommClient::connectToServer()
{
    if (d->m_connectionState == ConnectionState::Connected ||
        d->m_connectionState == ConnectionState::Connecting) {
        return;
    }

    if (d->m_clientState.activeChannelId.trimmed().isEmpty()) {
        return;
    }

    if (d->m_appConfig.messagesCollectionId.trimmed().isEmpty()) {
        emit errorOccurred("Cannot connect to server: messages collection is not configured.");
        return;
    }

    if (d->m_realtime == nullptr) {
        emit errorOccurred("Cannot connect to server: realtime dependency is missing.");
        return;
    }

    setConnectionState(ConnectionState::Connecting);

    const QString channel =
        QString("databases.%1.collections.%2.documents")
            .arg(d->m_baseConfig.dbId, d->m_appConfig.messagesCollectionId);

    d->m_realtime->connectToChannels({channel});
}

void AppcommClient::disconnectFromServer()
{
    if (d->m_connectionState == ConnectionState::Disconnected ||
        d->m_connectionState == ConnectionState::Disconnecting) {
        return;
    }

    if (d->m_realtime == nullptr) {
        setConnectionState(ConnectionState::Disconnected);
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

// Authentication

void AppcommClient::createGuestSession()
{
    if (d->m_client == nullptr) {
        emit errorOccurred("Cannot create guest session: client dependency is missing.");
        return;
    }

    d->m_pendingRequest = Private::PendingRequest::Login;
    d->m_client->createAnonymousSession(d->m_baseConfig);
}

void AppcommClient::createEmailSession(const QString &email, const QString &password)
{
    if (d->m_client == nullptr) {
        emit errorOccurred("Cannot create email session: client dependency is missing.");
        return;
    }

    d->m_pendingRequest = Private::PendingRequest::Login;
    d->m_client->createEmailSession(d->m_baseConfig, email, password);
}

void AppcommClient::logout()
{
    if (d->m_client != nullptr && !d->m_sessionInfo.sessionId.trimmed().isEmpty()) {
        d->m_client->deleteSession(d->m_baseConfig, d->m_sessionInfo.sessionId);
    }

    if (d->m_connectionState == ConnectionState::Connected ||
        d->m_connectionState == ConnectionState::Connecting) {
        disconnectFromServer();
    }

    d->m_sessionInfo = {};
    d->m_clientState = ClientState();
    d->m_messageProcessor->reset(QString());
    d->m_pendingRequest = Private::PendingRequest::None;

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

// Channel

void AppcommClient::joinChannel(const QString &channelId)
{
    const QString trimmedChannelId = channelId.trimmed();

    if (trimmedChannelId.isEmpty()) {
        emit errorOccurred("Channel ID cannot be empty.");
        return;
    }

    d->m_clientState.activeChannelId = trimmedChannelId;
    d->m_messageProcessor->reset(trimmedChannelId);

    emit joinedChannel(trimmedChannelId);

    loadChannelMessages();
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

    if (d->m_connectionState == ConnectionState::Connected) {
        disconnectFromServer();
    }
}

QString AppcommClient::activeChannel() const
{
    return d->m_clientState.activeChannelId;
}

// Messaging

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

    if (d->m_rateLimiter == nullptr) {
        emit errorOccurred("Cannot send message: rate limiter dependency is missing.");
        return;
    }

    if (!d->m_rateLimiter->allowRequest()) {
        emit errorOccurred("Rate limit exceeded.");
        return;
    }

    if (d->m_appConfig.incomingMessagesCollectionId.trimmed().isEmpty()) {
        emit errorOccurred("Cannot send message: incoming messages collection is not configured.");
        return;
    }

    if (d->m_client == nullptr) {
        emit errorOccurred("Cannot send message: client dependency is missing.");
        return;
    }

    appcomm::model::PendingMessage msg;
    msg.channelId = d->m_clientState.activeChannelId;
    msg.senderId = d->m_sessionInfo.userId;
    msg.messageId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    msg.timestamp = QDateTime::currentDateTimeUtc();
    msg.payload = payload;

    const appwritesdk::ConnectionConfig config =
        d->configForCollection(d->m_appConfig.incomingMessagesCollectionId);

    d->m_client->createDocument(config, msg.toJson());
}

void AppcommClient::loadMembership()
{
    if (d->m_sessionInfo.userId.trimmed().isEmpty()) {
        emit errorOccurred("Cannot load membership: missing user ID.");
        return;
    }

    if (d->m_appConfig.membersCollectionId.trimmed().isEmpty()) {
        emit errorOccurred("Cannot load membership: members collection is not configured.");
        return;
    }

    if (d->m_client == nullptr) {
        emit errorOccurred("Cannot load membership: client dependency is missing.");
        return;
    }

    const appwritesdk::ConnectionConfig config =
        d->configForCollection(d->m_appConfig.membersCollectionId);

    QJsonArray queries;
    queries.append(QString("{\"method\":\"equal\",\"attribute\":\"userId\",\"values\":[\"%1\"]}")
                       .arg(d->m_sessionInfo.userId));
    queries.append("{\"method\":\"limit\",\"values\":[1]}");

    d->m_pendingRequest = Private::PendingRequest::LoadMembership;
    d->m_client->listDocuments(config, queries);
}

void AppcommClient::loadChannelMessages(int limit)
{
    if (limit <= 0) {
        limit = 50;
    }

    if (d->m_clientState.activeChannelId.trimmed().isEmpty()) {
        emit errorOccurred("Cannot load messages: no active channel.");
        return;
    }

    if (d->m_appConfig.messagesCollectionId.trimmed().isEmpty()) {
        emit errorOccurred("Cannot load messages: messages collection is not configured.");
        return;
    }

    if (d->m_client == nullptr) {
        emit errorOccurred("Cannot load messages: client dependency is missing.");
        return;
    }

    const appwritesdk::ConnectionConfig config =
        d->configForCollection(d->m_appConfig.messagesCollectionId);

    QJsonArray queries;
    queries.append(QString("{\"method\":\"equal\",\"attribute\":\"channelId\",\"values\":[\"%1\"]}")
                       .arg(d->m_clientState.activeChannelId));
    queries.append("{\"method\":\"orderAsc\",\"attribute\":\"sequenceNumber\"}");
    queries.append(QString("{\"method\":\"limit\",\"values\":[%1]}").arg(limit));

    d->m_pendingRequest = Private::PendingRequest::LoadMessages;
    d->m_client->listDocuments(config, queries);
}

// Recovery slots

void AppcommClient::onMessagesRecovered(const QJsonArray &messages)
{
    handleMessageDocuments(messages);
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

// HTTP slots

void AppcommClient::onRequestSuccess(const QJsonObject &data)
{
    if (d->m_pendingRequest == Private::PendingRequest::Login && isSessionResponse(data)) {
        d->m_pendingRequest = Private::PendingRequest::None;

        d->m_sessionInfo.sessionId = data.value("$id").toString().trimmed();
        d->m_sessionInfo.userId = data.value("userId").toString().trimmed();

        d->m_sessionInfo.authType =
            (data.value("provider").toString().trimmed().toLower() == "email")
                ? model::AuthType::Email
                : model::AuthType::Guest;

        d->m_sessionInfo.createdAt =
            QDateTime::fromString(data.value("$createdAt").toString(), Qt::ISODate);

        d->m_sessionInfo.expiresAt =
            QDateTime::fromString(data.value("expire").toString(), Qt::ISODate);

        d->m_clientState.authenticated = true;
        emit authenticationStateChanged();

        loadMembership();
        return;
    }

    if (d->m_pendingRequest == Private::PendingRequest::LoadMembership) {
        d->m_pendingRequest = Private::PendingRequest::None;

        const QJsonArray documents = data.value("documents").toArray();
        if (documents.isEmpty()) {
            emit errorOccurred("No membership found for current user.");
            return;
        }

        const model::ChannelMember member =
            model::ChannelMember::fromJson(documents.first().toObject());

        if (member.userId.trimmed().isEmpty() || member.channelId.trimmed().isEmpty()) {
            emit errorOccurred("Invalid membership document.");
            return;
        }

        d->m_clientState.activeChannelId = member.channelId;
        d->m_messageProcessor->reset(member.channelId);

        emit joinedChannel(member.channelId);

        loadChannelMessages();
        return;
    }

    if (d->m_pendingRequest == Private::PendingRequest::LoadMessages) {
        d->m_pendingRequest = Private::PendingRequest::None;

        handleMessageDocuments(data.value("documents").toArray());

        if (d->m_connectionState == ConnectionState::Disconnected) {
            connectToServer();
        }

        return;
    }
}

void AppcommClient::onRequestError(int code, const QString &message)
{
    Q_UNUSED(code);

    d->m_pendingRequest = Private::PendingRequest::None;
    emit errorOccurred(message);
}

// Realtime slots

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
    const QJsonObject payload = extractRealtimePayload(event);
    if (payload.isEmpty()) {
        return;
    }

    handleIncomingMessage(appcomm::model::Message::fromJson(payload));
}

void AppcommClient::onErrorOccurred(const QString &error)
{
    setConnectionState(ConnectionState::Failed);
    emit errorOccurred(error);
}

// Internal

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
    const std::optional<model::Message> processed =
        d->m_messageProcessor->processIncoming(message);

    if (!processed.has_value()) {
        return;
    }

    d->m_recentMessageCache.addMessage(*processed);
    emit messageReceived(*processed);
}

void AppcommClient::handleMessageDocuments(const QJsonArray &documents)
{
    for (const QJsonValue &value : documents) {
        if (!value.isObject()) {
            continue;
        }

        handleIncomingMessage(model::Message::fromJson(value.toObject()));
    }
}
