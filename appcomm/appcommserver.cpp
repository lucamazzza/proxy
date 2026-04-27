/*!
 * @file appcommserver.cpp
 * @brief Implementation of server-side channel, user, message, and membership flows.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#include "appcommserver.h"
#include "appwritesdk.h"
#include "membershipservice.h"
#include "messagequeryservice.h"
#include "realtime.h"
#include "model.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QQueue>
#include <QTimer>
#include <QUuid>

using namespace appcomm::server;

namespace {

struct BootstrapAttributeDef {
    QString type;
    QString key;
    bool required;
    QJsonObject options;
};

struct BootstrapIndexDef {
    QString key;
    QString type;
    QStringList attributes;
};

QList<BootstrapAttributeDef> messageAttributes() {
    return {
        {"string", "channelId", APPCOMM_ATTR_REQUIRED, QJsonObject{{"size", 128}}},
        {"string", "senderId", APPCOMM_ATTR_REQUIRED, QJsonObject{{"size", 128}}},
        {"string", "messageId", APPCOMM_ATTR_REQUIRED, QJsonObject{{"size", 128}}},
        {"integer", "sequenceNumber", APPCOMM_ATTR_REQUIRED, QJsonObject{{"min", 0}}},
        {"datetime", "timestamp", APPCOMM_ATTR_REQUIRED, QJsonObject()},
        {"string", "payload", APPCOMM_ATTR_REQUIRED, QJsonObject{{"size", 10000}}},
        {"boolean", "isEcho", APPCOMM_ATTR_REQUIRED, QJsonObject()}
    };
}

QList<BootstrapIndexDef> messageIndexes() {
    return {
        {"idx_channel", "key", {"channelId"}},
        {"idx_timestamp", "key", {"timestamp"}},
        {"idx_sequence", "key", {"sequenceNumber"}},
        {"idx_message_unique", "unique", {"messageId"}},
        {"idx_channel_sequence_unique", "unique", {"channelId", "sequenceNumber"}},
        {"idx_channel_timestamp", "key", {"channelId", "timestamp"}}
    };
}

bool isAlreadyExistsError(int code, const QString &message) {
    if (code == 409) {
        return true;
    }
    const QString normalized = message.toLower();
    return normalized.contains("already exists") || normalized.contains("duplicate");
}

bool isBootstrapLimitError(const QString &message) {
    const QString normalized = message.toLower();
    return (normalized.contains("maximum number or size of attributes")
            || normalized.contains("maximum number of attributes")
            || normalized.contains("maximum number of indexes")
            || normalized.contains("maximum number of index"))
           && normalized.contains("reached");
}

bool shouldRetryIndexCreation(const QString &message) {
    const QString normalized = message.toLower();
    return normalized.contains("not yet available")
           || (normalized.contains("attribute") && normalized.contains("available"));
}

bool isDatabaseLimitReached(const QString &message) {
    const QString normalized = message.toLower();
    return normalized.contains("maximum number of databases")
           || normalized.contains("max databases")
           || normalized.contains("only one database")
           || normalized.contains("one database");
}

appcomm::model::Message makeSequencedMessage(const appcomm::model::PendingMessage &pending,
                                             qint64 sequenceNumber,
                                             bool isEcho)
{
    appcomm::model::Message msg;
    msg.channelId = pending.channelId;
    msg.senderId = pending.senderId;
    msg.messageId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    msg.sequenceNumber = sequenceNumber;
    msg.timestamp = QDateTime::currentDateTimeUtc();
    msg.payload = pending.payload;
    msg.isEcho = isEcho;
    return msg;
}

} // namespace

class AppcommServer::Private {
public:
    enum class InitStep {
        Idle,
        CreatingDatabase,
        ListingDatabases,
        CreatingCollection,
        CreatingAttribute,
        CreatingIndex
    };

    enum class AsyncOperation {
        None,
        DeletingChannelListingDocs,
        DeletingChannelDeletingDocs,
        AddingMember,
        RemovingMemberListingDocs,
        RemovingMemberDeletingDoc,
        ListingMembers,
        DeletingUser,
        DeletingMessageListingDoc,
        DeletingMessageDeletingDoc,
        ResolvingNextSequence,
        CreatingFinalUserMessage,
        CreatingEchoMessage,
        DeletingIncomingMessage
    };

    model::AppCommConfig config;
    appwritesdk::ConnectionConfig sdkConfig;
    QNetworkAccessManager *networkManager;
    appwritesdk::Server *server;
    MembershipService *membershipService;
    MessageQueryService *messageQueryService;
    Realtime *realtime;
    bool initialized;
    QHash<QString, model::Channel> channels;
    QHash<QString, model::User> users;
    InitStep initStep = InitStep::Idle;
    QList<BootstrapAttributeDef> bootstrapAttributes;
    QList<BootstrapIndexDef> bootstrapIndexes;
    int currentAttributeIndex = 0;
    int currentIndexIndex = 0;
    int currentIndexAttempt = 0;
    QStringList docsToDeleteInChannel;
    AsyncOperation activeOperation = AsyncOperation::None;
    model::ChannelMember pendingMemberToAdd;
    QString pendingDeletedUserId;
    QString pendingDeletedMessageId;

    QQueue<QPair<QString, model::PendingMessage>> incomingQueue;
    QString currentIncomingDocumentId;
    model::PendingMessage currentIncomingMessage;
    qint64 nextSequenceNumber = 0;

    Private()
        : networkManager(nullptr)
        , server(nullptr)
        , membershipService(nullptr)
        , messageQueryService(nullptr)
        , realtime(nullptr)
        , initialized(false)
    {
    }

    ~Private() {
    }

    appwritesdk::ConnectionConfig configForCollection(const QString &collectionId) const
    {
        appwritesdk::ConnectionConfig configCopy = sdkConfig;
        configCopy.collectionId = collectionId;
        return configCopy;
    }

    void clearCurrentIncoming()
    {
        currentIncomingDocumentId.clear();
        currentIncomingMessage = model::PendingMessage();
        nextSequenceNumber = 0;
    }
};

AppcommServer::AppcommServer(QObject *parent)
    : QObject{parent}
    , d(new Private())
{
    d->networkManager = new QNetworkAccessManager(this);
    d->server = new appwritesdk::Server(d->networkManager, this);
    d->membershipService = new MembershipService(this);
    d->messageQueryService = new MessageQueryService(this);

    connect(d->server, &appwritesdk::Server::requestSuccess,
            this, &AppcommServer::onServerRequestSuccess);
    connect(d->server, &appwritesdk::Server::requestError,
            this, &AppcommServer::onServerRequestError);
}

AppcommServer::~AppcommServer() {
    delete d;
}

void AppcommServer::configure(const model::AppCommConfig &config) {
    d->config = config;
    d->sdkConfig.endpoint = config.endpoint;
    d->sdkConfig.projectId = config.projectId;
    d->sdkConfig.apiKey = config.apiKey;
    d->sdkConfig.dbId = config.databaseId;
    d->sdkConfig.collectionId = config.messagesCollectionId;

    if (d->realtime) {
        delete d->realtime;
    }

    appwritesdk::ConnectionConfig incomingConfig = d->configForCollection(config.incomingMessagesCollectionId);
    d->realtime = new Realtime(incomingConfig, this);

    const QStringList channels{
        QString("databases.%1.collections.%2.documents")
        .arg(config.databaseId, config.incomingMessagesCollectionId)
    };

    QObject::connect(d->realtime, &Realtime::eventReceived, this, [this](const QJsonObject &event) {
        QJsonObject eventData = event;
        if (eventData.contains("data") && eventData.value("data").isObject()) {
            eventData = eventData.value("data").toObject();
        }

        const QJsonValue payloadValue = eventData.value("payload");
        if (!payloadValue.isObject()) {
            return;
        }

        QJsonObject payloadObj = payloadValue.toObject();
        if (payloadObj.contains("data") && payloadObj.value("data").isObject()) {
            payloadObj = payloadObj.value("data").toObject();
        }

        const QString documentId = payloadObj.value("$id").toString().trimmed();
        const model::PendingMessage pending = model::PendingMessage::fromJson(payloadObj);

        if (documentId.isEmpty() || !pending.isValid()) {
            return;
        }

        enqueueIncomingMessage(documentId, pending);
    });

    d->realtime->connectToChannels(channels);
    emit configured();
}

void AppcommServer::initialize() {
    if (d->sdkConfig.endpoint.trimmed().isEmpty()
        || d->sdkConfig.projectId.trimmed().isEmpty()
        || d->sdkConfig.apiKey.trimmed().isEmpty()
        || d->sdkConfig.dbId.trimmed().isEmpty()
        || d->config.messagesCollectionId.trimmed().isEmpty()) {
        failInitialization(400, "Server is not configured");
        return;
    }

    d->initialized = false;
    d->bootstrapAttributes = messageAttributes();
    d->bootstrapIndexes = messageIndexes();
    d->currentAttributeIndex = 0;
    d->currentIndexIndex = 0;
    d->currentIndexAttempt = 0;
    d->initStep = Private::InitStep::CreatingDatabase;
    d->server->createDatabase(d->sdkConfig, d->sdkConfig.dbId);
}

void AppcommServer::setupCollection() {
    const QJsonArray permissions = {
        "read(\"any\")",
        "create(\"any\")",
        "update(\"any\")",
        "delete(\"any\")"
    };
    d->server->createCollection(d->sdkConfig, "messages", permissions);
}

void AppcommServer::createMessageAttributes() {
    for (const BootstrapAttributeDef &attribute : messageAttributes()) {
        d->server->createAttribute(d->sdkConfig,
                                   attribute.type,
                                   attribute.key,
                                   attribute.required,
                                   attribute.options);
    }
}

void AppcommServer::createIndexes() {
    for (const BootstrapIndexDef &index : messageIndexes()) {
        d->server->createIndex(d->sdkConfig, index.key, index.type, index.attributes);
    }
}

void AppcommServer::continueInitializationAfterDatabase() {
    d->initStep = Private::InitStep::CreatingCollection;
    setupCollection();
}

void AppcommServer::requestNextBootstrapAttribute() {
    if (d->currentAttributeIndex >= d->bootstrapAttributes.size()) {
        d->currentIndexIndex = 0;
        d->currentIndexAttempt = 0;
        requestNextBootstrapIndex();
        return;
    }

    const BootstrapAttributeDef attribute = d->bootstrapAttributes.at(d->currentAttributeIndex);
    d->initStep = Private::InitStep::CreatingAttribute;
    d->server->createAttribute(d->sdkConfig,
                               attribute.type,
                               attribute.key,
                               attribute.required,
                               attribute.options);
}

void AppcommServer::requestCurrentBootstrapIndex() {
    if (d->currentIndexIndex >= d->bootstrapIndexes.size()) {
        completeInitialization();
        return;
    }

    const BootstrapIndexDef index = d->bootstrapIndexes.at(d->currentIndexIndex);
    d->initStep = Private::InitStep::CreatingIndex;
    d->currentIndexAttempt += 1;
    d->server->createIndex(d->sdkConfig, index.key, index.type, index.attributes);
}

void AppcommServer::requestNextBootstrapIndex() {
    if (d->currentIndexIndex >= d->bootstrapIndexes.size()) {
        completeInitialization();
        return;
    }

    d->currentIndexAttempt = 0;
    requestCurrentBootstrapIndex();
}

void AppcommServer::completeInitialization() {
    d->initStep = Private::InitStep::Idle;
    if (!d->initialized) {
        d->initialized = true;
        emit initialized();
    }
}

void AppcommServer::failInitialization(int code, const QString &message) {
    d->initStep = Private::InitStep::Idle;
    emit initializationError(code, message);
}

void AppcommServer::enqueueIncomingMessage(const QString &documentId, const model::PendingMessage &message)
{
    d->incomingQueue.enqueue(qMakePair(documentId, message));
    processNextIncomingMessage();
}

void AppcommServer::processNextIncomingMessage()
{
    if (d->activeOperation != Private::AsyncOperation::None) {
        return;
    }

    if (d->incomingQueue.isEmpty()) {
        return;
    }

    const auto next = d->incomingQueue.dequeue();
    d->currentIncomingDocumentId = next.first;
    d->currentIncomingMessage = next.second;

    d->activeOperation = Private::AsyncOperation::ResolvingNextSequence;
    const QJsonArray queries = d->messageQueryService->lastSequenceForChannel(d->currentIncomingMessage.channelId);
    d->server->listDocuments(d->configForCollection(d->config.messagesCollectionId), queries);
}

void AppcommServer::createChannel(const model::Channel &channel) {
    if (!channel.isValid()) {
        emit channelError(400, "Invalid channel data");
        return;
    }

    d->channels[channel.channelId] = channel;
    emit channelCreated(channel);
}

void AppcommServer::deleteChannel(const QString &channelId) {
    if (channelId.isEmpty()) {
        emit channelError(400, "Channel ID cannot be empty");
        return;
    }
    if (d->activeOperation != Private::AsyncOperation::None) {
        emit operationError(409, "Another server operation is already in progress");
        return;
    }

    m_deletingChannelId = channelId;
    m_originalCollectionId = d->sdkConfig.collectionId;
    const QJsonArray queries = d->messageQueryService->channelDocuments(channelId, 100);
    d->docsToDeleteInChannel.clear();
    d->activeOperation = Private::AsyncOperation::DeletingChannelListingDocs;
    d->sdkConfig.collectionId = d->config.messagesCollectionId;
    d->server->listDocuments(d->sdkConfig, queries);
}

void AppcommServer::listChannels() {
    emit channelsListed(d->channels.values());
}

void AppcommServer::createUser(const QString &email, const QString &password, const QString &name) {
    if (email.isEmpty() || password.isEmpty()) {
        emit userError(400, "Email and Password cannot be empty");
        return;
    }

    d->server->createUser(d->sdkConfig, email, password, name);
}

void AppcommServer::deleteUser(const QString &userId) {
    if (userId.isEmpty()) {
        emit userError(400, "User ID cannot be empty");
        return;
    }
    if (d->activeOperation != Private::AsyncOperation::None) {
        emit userError(409, "Another server operation is already in progress");
        return;
    }

    d->activeOperation = Private::AsyncOperation::DeletingUser;
    d->pendingDeletedUserId = userId;
    d->server->deleteUser(d->sdkConfig, userId);
}

void AppcommServer::listUsers() {
    QJsonArray queries;
    queries.append("limit(100)");
    d->server->listUsers(d->sdkConfig, queries);
}

void AppcommServer::broadcastMessage(const model::Message &message) {
    if (!message.channelId.trimmed().isEmpty()
        && !message.senderId.trimmed().isEmpty()
        && message.timestamp.isValid()) {
        model::PendingMessage pending;
        pending.channelId = message.channelId;
        pending.senderId = message.senderId;
        pending.messageId = message.messageId.trimmed().isEmpty()
                                ? QUuid::createUuid().toString(QUuid::WithoutBraces)
                                : message.messageId;
        pending.timestamp = message.timestamp;
        pending.payload = message.payload;

        enqueueIncomingMessage(QString(), pending);
        return;
    }

    emit messageError(400, "Invalid Message");
}

void AppcommServer::getChannelMessages(const QString &channelId, int limit) {
    if (channelId.isEmpty()) {
        emit messageError(400, "Channel ID cannot be empty");
        return;
    }

    const QJsonArray queries = d->messageQueryService->channelMessages(channelId, limit);
    d->server->listDocuments(d->configForCollection(d->config.messagesCollectionId), queries);
}

void AppcommServer::deleteMessage(const QString &messageId) {
    if (messageId.isEmpty()) {
        emit messageError(400, "Message ID cannot be empty");
        return;
    }
    if (d->activeOperation != Private::AsyncOperation::None) {
        emit messageError(409, "Another server operation is already in progress");
        return;
    }

    m_originalCollectionId = d->sdkConfig.collectionId;
    d->activeOperation = Private::AsyncOperation::DeletingMessageListingDoc;
    d->pendingDeletedMessageId = messageId;
    d->sdkConfig.collectionId = d->config.messagesCollectionId;
    const QJsonArray queries = d->messageQueryService->messageDocuments(messageId, 1);
    d->server->listDocuments(d->sdkConfig, queries);
}

void AppcommServer::addChannelMember(const QString &channelId, const QString &userId) {
    if (channelId.isEmpty() || userId.isEmpty()) {
        emit membershipError(400, "Channel ID and User ID cannot be empty");
        return;
    }
    if (d->activeOperation != Private::AsyncOperation::None) {
        emit membershipError(409, "Another server operation is already in progress");
        return;
    }

    const model::ChannelMember member = d->membershipService->createActiveMember(
        channelId,
        userId,
        QDateTime::currentDateTimeUtc());

    d->pendingMemberToAdd = member;
    d->activeOperation = Private::AsyncOperation::AddingMember;
    d->server->createDocument(d->configForCollection(d->config.membersCollectionId), member.toJson());
}

void AppcommServer::removeChannelMember(const QString &channelId, const QString &userId) {
    if (channelId.isEmpty() || userId.isEmpty()) {
        emit membershipError(400, "Channel ID and User ID cannot be empty");
        return;
    }
    if (d->activeOperation != Private::AsyncOperation::None) {
        emit membershipError(409, "Another server operation is already in progress");
        return;
    }

    m_removingChannelId = channelId;
    m_removingUserId = userId;
    m_originalCollectionId = d->sdkConfig.collectionId;
    const QJsonArray queries = d->messageQueryService->channelMemberDocuments(channelId, userId, 1);
    d->activeOperation = Private::AsyncOperation::RemovingMemberListingDocs;
    d->sdkConfig.collectionId = d->config.membersCollectionId;
    d->server->listDocuments(d->sdkConfig, queries);
}

void AppcommServer::getChannelMembers(const QString &channelId) {
    if (channelId.isEmpty()) {
        emit membershipError(400, "Channel ID cannot be empty");
        return;
    }
    if (d->activeOperation != Private::AsyncOperation::None) {
        emit membershipError(409, "Another server operation is already in progress");
        return;
    }

    m_queryingChannelId = channelId;
    m_originalCollectionId = d->sdkConfig.collectionId;
    const QJsonArray queries = d->messageQueryService->channelDocuments(channelId);
    d->activeOperation = Private::AsyncOperation::ListingMembers;
    d->sdkConfig.collectionId = d->config.membersCollectionId;
    d->server->listDocuments(d->sdkConfig, queries);
}

bool AppcommServer::isInitialized() const {
    return d->initialized;
}

QList<appcomm::model::Channel> AppcommServer::channels() const {
    return d->channels.values();
}

appcomm::model::Channel AppcommServer::getChannel(const QString &channelId) const {
    return d->channels.value(channelId);
}

bool AppcommServer::handleOperationSuccess(const QJsonObject &data) {
    switch (d->activeOperation) {
    case Private::AsyncOperation::None:
        return false;

    case Private::AsyncOperation::ResolvingNextSequence: {
        const QJsonArray docs = data.value("documents").toArray();
        if (!docs.isEmpty()) {
            const model::Message lastMessage = model::Message::fromJson(docs.first().toObject());
            d->nextSequenceNumber = lastMessage.isValid() ? lastMessage.sequenceNumber + 1 : 0;
        } else {
            d->nextSequenceNumber = 0;
        }

        const model::Message userMessage =
            makeSequencedMessage(d->currentIncomingMessage, d->nextSequenceNumber, false);

        d->activeOperation = Private::AsyncOperation::CreatingFinalUserMessage;
        d->server->createDocument(d->configForCollection(d->config.messagesCollectionId), userMessage.toJson());
        return true;
    }

    case Private::AsyncOperation::CreatingFinalUserMessage: {
        const model::Message createdMessage = model::Message::fromJson(data);
        if (createdMessage.isValid()) {
            emit messageBroadcasted(createdMessage);
        }

        const model::Message echoMessage =
            makeSequencedMessage(d->currentIncomingMessage, d->nextSequenceNumber + 1, true);

        d->activeOperation = Private::AsyncOperation::CreatingEchoMessage;
        d->server->createDocument(d->configForCollection(d->config.messagesCollectionId), echoMessage.toJson());
        return true;
    }

    case Private::AsyncOperation::CreatingEchoMessage: {
        const model::Message createdEcho = model::Message::fromJson(data);
        if (createdEcho.isValid()) {
            emit messageBroadcasted(createdEcho);
        }

        if (d->currentIncomingDocumentId.trimmed().isEmpty()) {
            d->activeOperation = Private::AsyncOperation::None;
            d->clearCurrentIncoming();
            processNextIncomingMessage();
            return true;
        }

        d->activeOperation = Private::AsyncOperation::DeletingIncomingMessage;
        d->server->deleteDocument(
            d->configForCollection(d->config.incomingMessagesCollectionId),
            d->currentIncomingDocumentId);
        return true;
    }

    case Private::AsyncOperation::DeletingIncomingMessage:
        d->activeOperation = Private::AsyncOperation::None;
        d->clearCurrentIncoming();
        processNextIncomingMessage();
        return true;

    case Private::AsyncOperation::DeletingChannelListingDocs: {
        const QJsonArray docs = data.value("documents").toArray();
        if (docs.isEmpty()) {
            d->sdkConfig.collectionId = m_originalCollectionId;
            d->docsToDeleteInChannel.clear();
            d->activeOperation = Private::AsyncOperation::None;
            d->channels.remove(m_deletingChannelId);
            emit channelDeleted(m_deletingChannelId);
            return true;
        }

        d->docsToDeleteInChannel.clear();
        for (const QJsonValue &doc : docs) {
            const QString docId = doc.toObject().value("$id").toString().trimmed();
            if (!docId.isEmpty()) {
                d->docsToDeleteInChannel.append(docId);
            }
        }

        if (d->docsToDeleteInChannel.isEmpty()) {
            d->sdkConfig.collectionId = m_originalCollectionId;
            d->activeOperation = Private::AsyncOperation::None;
            emit channelError(500, "Unable to resolve message document IDs while deleting channel");
            return true;
        }

        d->activeOperation = Private::AsyncOperation::DeletingChannelDeletingDocs;
        d->server->deleteDocument(d->sdkConfig, d->docsToDeleteInChannel.takeFirst());
        return true;
    }

    case Private::AsyncOperation::DeletingChannelDeletingDocs:
        if (!d->docsToDeleteInChannel.isEmpty()) {
            d->server->deleteDocument(d->sdkConfig, d->docsToDeleteInChannel.takeFirst());
            return true;
        } else {
            d->activeOperation = Private::AsyncOperation::DeletingChannelListingDocs;
            const QJsonArray queries = d->messageQueryService->channelDocuments(m_deletingChannelId, 100);
            d->server->listDocuments(d->sdkConfig, queries);
            return true;
        }

    case Private::AsyncOperation::AddingMember: {
        model::ChannelMember member = model::ChannelMember::fromJson(data);
        if (member.userId.trimmed().isEmpty()) {
            member = d->pendingMemberToAdd;
        }
        d->pendingMemberToAdd = model::ChannelMember();
        d->activeOperation = Private::AsyncOperation::None;
        emit memberAdded(member);
        return true;
    }

    case Private::AsyncOperation::RemovingMemberListingDocs: {
        const QJsonArray docs = data.value("documents").toArray();
        if (docs.isEmpty()) {
            d->sdkConfig.collectionId = m_originalCollectionId;
            d->activeOperation = Private::AsyncOperation::None;
            emit memberRemoved(m_removingChannelId, m_removingUserId);
            return true;
        }

        const QString docId = d->membershipService->resolveDocumentId(docs.first().toObject());
        if (docId.isEmpty()) {
            d->sdkConfig.collectionId = m_originalCollectionId;
            d->activeOperation = Private::AsyncOperation::None;
            emit membershipError(500, "Unable to resolve member document ID");
            return true;
        }

        d->activeOperation = Private::AsyncOperation::RemovingMemberDeletingDoc;
        d->server->deleteDocument(d->sdkConfig, docId);
        return true;
    }

    case Private::AsyncOperation::RemovingMemberDeletingDoc:
        d->sdkConfig.collectionId = m_originalCollectionId;
        d->activeOperation = Private::AsyncOperation::None;
        emit memberRemoved(m_removingChannelId, m_removingUserId);
        return true;

    case Private::AsyncOperation::ListingMembers: {
        const QJsonArray docs = data.value("documents").toArray();
        const QList<model::ChannelMember> members = d->membershipService->parseMembers(docs);
        d->sdkConfig.collectionId = m_originalCollectionId;
        d->activeOperation = Private::AsyncOperation::None;
        emit membersListed(m_queryingChannelId, members);
        return true;
    }

    case Private::AsyncOperation::DeletingUser: {
        const QString deletedUserId = d->pendingDeletedUserId;
        d->pendingDeletedUserId.clear();
        d->users.remove(deletedUserId);
        d->activeOperation = Private::AsyncOperation::None;
        emit userDeleted(deletedUserId);
        return true;
    }

    case Private::AsyncOperation::DeletingMessageListingDoc: {
        const QJsonArray docs = data.value("documents").toArray();
        if (docs.isEmpty()) {
            d->sdkConfig.collectionId = m_originalCollectionId;
            d->activeOperation = Private::AsyncOperation::None;
            const QString missingMessageId = d->pendingDeletedMessageId;
            d->pendingDeletedMessageId.clear();
            emit messageError(404, QString("Message not found: %1").arg(missingMessageId));
            return true;
        }

        const QString docId = docs.first().toObject().value("$id").toString().trimmed();
        if (docId.isEmpty()) {
            d->sdkConfig.collectionId = m_originalCollectionId;
            d->activeOperation = Private::AsyncOperation::None;
            d->pendingDeletedMessageId.clear();
            emit messageError(500, "Unable to resolve message document ID");
            return true;
        }

        d->activeOperation = Private::AsyncOperation::DeletingMessageDeletingDoc;
        d->server->deleteDocument(d->sdkConfig, docId);
        return true;
    }

    case Private::AsyncOperation::DeletingMessageDeletingDoc: {
        const QString deletedMessageId = d->pendingDeletedMessageId;
        d->pendingDeletedMessageId.clear();
        d->sdkConfig.collectionId = m_originalCollectionId;
        d->activeOperation = Private::AsyncOperation::None;
        emit messageDeleted(deletedMessageId);
        return true;
    }
    }

    return false;
}

bool AppcommServer::handleOperationError(int code, const QString &message) {
    switch (d->activeOperation) {
    case Private::AsyncOperation::None:
        return false;

    case Private::AsyncOperation::ResolvingNextSequence:
    case Private::AsyncOperation::CreatingFinalUserMessage:
    case Private::AsyncOperation::CreatingEchoMessage:
    case Private::AsyncOperation::DeletingIncomingMessage:
        d->activeOperation = Private::AsyncOperation::None;
        d->clearCurrentIncoming();
        emit messageError(code, message);
        processNextIncomingMessage();
        return true;

    case Private::AsyncOperation::DeletingChannelListingDocs:
    case Private::AsyncOperation::DeletingChannelDeletingDocs:
        d->docsToDeleteInChannel.clear();
        d->sdkConfig.collectionId = m_originalCollectionId;
        d->activeOperation = Private::AsyncOperation::None;
        emit channelError(code, message);
        return true;

    case Private::AsyncOperation::AddingMember:
        d->pendingMemberToAdd = model::ChannelMember();
        d->activeOperation = Private::AsyncOperation::None;
        emit membershipError(code, message);
        return true;

    case Private::AsyncOperation::RemovingMemberListingDocs:
    case Private::AsyncOperation::RemovingMemberDeletingDoc:
    case Private::AsyncOperation::ListingMembers:
        d->sdkConfig.collectionId = m_originalCollectionId;
        d->activeOperation = Private::AsyncOperation::None;
        emit membershipError(code, message);
        return true;

    case Private::AsyncOperation::DeletingUser:
        d->pendingDeletedUserId.clear();
        d->activeOperation = Private::AsyncOperation::None;
        emit userError(code, message);
        return true;

    case Private::AsyncOperation::DeletingMessageListingDoc:
    case Private::AsyncOperation::DeletingMessageDeletingDoc:
        d->pendingDeletedMessageId.clear();
        d->sdkConfig.collectionId = m_originalCollectionId;
        d->activeOperation = Private::AsyncOperation::None;
        emit messageError(code, message);
        return true;
    }

    return false;
}

bool AppcommServer::handleInitializationSuccess(const QJsonObject &data) {
    switch (d->initStep) {
    case Private::InitStep::CreatingDatabase:
        if (!data.contains("$id")) {
            return false;
        }
        continueInitializationAfterDatabase();
        return true;

    case Private::InitStep::ListingDatabases: {
        if (!data.contains("databases")) {
            return false;
        }

        const QJsonArray databases = data.value("databases").toArray();
        if (databases.isEmpty()) {
            failInitialization(404, "No existing database found to reuse");
            return true;
        }

        QString existingDatabaseId;
        for (const QJsonValue &databaseValue : databases) {
            const QString databaseId = databaseValue.toObject().value("$id").toString().trimmed();
            if (!databaseId.isEmpty()) {
                existingDatabaseId = databaseId;
                break;
            }
        }

        if (existingDatabaseId.isEmpty()) {
            failInitialization(404, "Unable to resolve existing database ID from Appwrite response");
            return true;
        }

        d->config.databaseId = existingDatabaseId;
        d->sdkConfig.dbId = existingDatabaseId;
        continueInitializationAfterDatabase();
        return true;
    }

    case Private::InitStep::CreatingCollection:
        if (!data.contains("$id") || !data.contains("$permissions")) {
            return false;
        }
        d->currentAttributeIndex = 0;
        requestNextBootstrapAttribute();
        return true;

    case Private::InitStep::CreatingAttribute:
        if (!data.contains("key") || !data.contains("type")) {
            return false;
        }
        d->currentAttributeIndex += 1;
        requestNextBootstrapAttribute();
        return true;

    case Private::InitStep::CreatingIndex:
        if (!data.contains("key") || !data.contains("status")) {
            return false;
        }
        d->currentIndexIndex += 1;
        d->currentIndexAttempt = 0;
        requestNextBootstrapIndex();
        return true;

    case Private::InitStep::Idle:
        break;
    }

    return false;
}

bool AppcommServer::handleInitializationError(int code, const QString &message) {
    switch (d->initStep) {
    case Private::InitStep::CreatingDatabase:
        if (isAlreadyExistsError(code, message)) {
            continueInitializationAfterDatabase();
            return true;
        }
        if (isDatabaseLimitReached(message)) {
            d->initStep = Private::InitStep::ListingDatabases;
            d->server->listDatabases(d->sdkConfig);
            return true;
        }
        failInitialization(code, message);
        return true;

    case Private::InitStep::ListingDatabases:
        failInitialization(code, message);
        return true;

    case Private::InitStep::CreatingCollection:
        if (isAlreadyExistsError(code, message)) {
            d->currentAttributeIndex = 0;
            requestNextBootstrapAttribute();
            return true;
        }
        failInitialization(code, message);
        return true;

    case Private::InitStep::CreatingAttribute:
        if (isAlreadyExistsError(code, message) || isBootstrapLimitError(message)) {
            d->currentAttributeIndex += 1;
            requestNextBootstrapAttribute();
            return true;
        }
        failInitialization(code, message);
        return true;

    case Private::InitStep::CreatingIndex:
        if (isAlreadyExistsError(code, message) || isBootstrapLimitError(message)) {
            d->currentIndexIndex += 1;
            d->currentIndexAttempt = 0;
            requestNextBootstrapIndex();
            return true;
        }
        if (shouldRetryIndexCreation(message) && d->currentIndexAttempt < 10) {
            QTimer::singleShot(1000, this, [this]() {
                if (d->initStep == Private::InitStep::CreatingIndex) {
                    requestCurrentBootstrapIndex();
                }
            });
            return true;
        }
        failInitialization(code, message);
        return true;

    case Private::InitStep::Idle:
        break;
    }

    return false;
}

void AppcommServer::onServerRequestSuccess(const QJsonObject &data) {
    if (handleInitializationSuccess(data)) {
        return;
    }
    if (handleOperationSuccess(data)) {
        return;
    }

    if (data.contains("email") && data.contains("$id")) {
        model::User user;
        user.userId = data.value("$id").toString();
        user.email = data.value("email").toString();
        d->users[user.userId] = user;
        emit userCreated(user);
        return;
    }

    if (data.contains("users")) {
        QJsonArray usersArray = data.value("users").toArray();
        QList<model::User> users;
        for (const QJsonValue &userValue : usersArray) {
            QJsonObject userObj = userValue.toObject();
            model::User user;
            user.userId = userObj.value("$id").toString();
            user.email = userObj.value("email").toString();
            users.append(user);
        }
        emit usersListed(users);
        return;
    }

    if (data.contains("documents")) {
        QJsonArray docs = data.value("documents").toArray();
        QList<model::Message> messages;
        for (const QJsonValue &docValue : docs) {
            model::Message msg = model::Message::fromJson(docValue.toObject());
            if (msg.isValid()) {
                messages.append(msg);
            }
        }
        emit messagesRetrieved(messages);
        return;
    }
}

void AppcommServer::onServerRequestError(int code, const QString &message) {
    if (handleInitializationError(code, message)) {
        return;
    }
    if (handleOperationError(code, message)) {
        return;
    }
    if (!d->initialized) {
        emit initializationError(code, message);
    } else {
        emit operationError(code, message);
    }
}