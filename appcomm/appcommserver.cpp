#include "appcommserver.h"
#include "appwritesdk.h"
#include "membershipservice.h"
#include "realtime.h"
#include "model.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>

using namespace appcomm::server;

namespace {

QString escapeQueryValue(const QString &value) {
    QString escaped = value;
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");
    return escaped;
}

QString equalQuery(const QString &key, const QString &value) {
    return QString("equal(\"%1\",[\"%2\"])").arg(key, escapeQueryValue(value));
}

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

}

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

    model::AppCommConfig config;
    appwritesdk::ConnectionConfig sdkConfig;
    QNetworkAccessManager *networkManager;
    appwritesdk::Server *server;
    MembershipService *membershipService;
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

    Private()
        : networkManager(nullptr)
        , server(nullptr)
        , membershipService(nullptr)
        , realtime(nullptr)
        , initialized(false)
    {}

    ~Private() {
    }
};

AppcommServer::AppcommServer(QObject *parent)
    : QObject{parent}
    , d(new Private())
{
    d->networkManager = new QNetworkAccessManager(this);
    d->server = new appwritesdk::Server(d->networkManager, this);
    d->membershipService = new MembershipService(this);
    connect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onServerRequestSuccess);
    connect(d->server, &appwritesdk::Server::requestError, this, &AppcommServer::onServerRequestError);
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
    d->realtime = new Realtime(d->sdkConfig, this);
    QStringList channels;
    channels.append(QString("databases.%1.collections.%2.documents")
                    .arg(config.databaseId, config.messagesCollectionId));
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
        model::Message msg = model::Message::fromJson(payloadObj);
        if (msg.isValid()) {
            emit messageBroadcasted(msg);
        }
    });
    d->realtime->connect(channels);
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
    m_deletingChannelId = channelId;
    m_originalCollectionId = d->sdkConfig.collectionId;
    QJsonArray queries;
    queries.append(equalQuery("channelId", channelId));
    queries.append("limit(100)");
    d->docsToDeleteInChannel.clear();
    d->sdkConfig.collectionId = d->config.messagesCollectionId;
    disconnect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onServerRequestSuccess);
    connect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onChannelDocumentsListed);
    d->server->listDocuments(d->sdkConfig, queries);
}

void AppcommServer::onChannelDocumentsListed(const QJsonObject &data) {
    QJsonArray docs = data.value("documents").toArray();
    if (docs.isEmpty()) {
        disconnect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onChannelDocumentsListed);
        connect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onServerRequestSuccess);
        d->sdkConfig.collectionId = m_originalCollectionId;
        d->channels.remove(m_deletingChannelId);
        emit channelDeleted(m_deletingChannelId);
        return;
    }
    for (const QJsonValue &doc : docs) {
        QString docId = doc.toObject().value("$id").toString();
        if (!docId.isEmpty()) {
            d->docsToDeleteInChannel.append(docId);
        }
    }
    disconnect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onChannelDocumentsListed);
    connect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onChannelDocumentDeleted);
    if (!d->docsToDeleteInChannel.isEmpty()) {
        QString docId = d->docsToDeleteInChannel.takeFirst();
        d->server->deleteDocument(d->sdkConfig, docId);
    }
}

void AppcommServer::onChannelDocumentDeleted(const QJsonObject &data) {
    Q_UNUSED(data);
    if (!d->docsToDeleteInChannel.isEmpty()) {
        QString docId = d->docsToDeleteInChannel.takeFirst();
        d->server->deleteDocument(d->sdkConfig, docId);
    } else {
        disconnect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onChannelDocumentDeleted);
        connect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onServerRequestSuccess);
        d->sdkConfig.collectionId = m_originalCollectionId;
        d->channels.remove(m_deletingChannelId);
        emit channelDeleted(m_deletingChannelId);
    }
}

void AppcommServer::listChannels() {
    QList<model::Channel> channelList = d->channels.values();
    emit channelsListed(channelList);
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
    d->server->deleteUser(d->sdkConfig, userId);
}

void AppcommServer::listUsers() {
    QJsonArray queries;
    // Parametrize limit
    queries.append("limit(100)");
    d->server->listUsers(d->sdkConfig, queries);
}

void AppcommServer::broadcastMessage(const model::Message &message) {
    if (!message.isValid()) {
        emit messageError(400, "Invalid Message");
        return;
    }
    auto oldCollectionId = d->sdkConfig.collectionId;
    d->sdkConfig.collectionId = d->config.messagesCollectionId;
    d->server->createDocument(d->sdkConfig, message.toJson());
    d->sdkConfig.collectionId = oldCollectionId;
}

void AppcommServer::getChannelMessages(const QString &channelId, int limit) {
    if (channelId.isEmpty()) {
        emit messageError(400, "Channel ID cannot be empty");
        return;
    }
    QJsonArray queries;
    queries.append(equalQuery("channelId", channelId));
    queries.append("orderDesc(\"timestamp\")");
    queries.append(QString("limit(%1)").arg(limit));
    auto oldCollectionId = d->sdkConfig.collectionId;
    d->sdkConfig.collectionId = d->config.messagesCollectionId;
    d->server->listDocuments(d->sdkConfig, queries);
    d->sdkConfig.collectionId = oldCollectionId;
}

void AppcommServer::deleteMessage(const QString &messageId) {
    if (messageId.isEmpty()) {
        emit messageError(400, "Message ID cannot be empty");
        return;
    }
    auto oldCollectionId = d->sdkConfig.collectionId;
    d->sdkConfig.collectionId = d->config.messagesCollectionId;
    d->server->deleteDocument(d->sdkConfig, messageId);
    d->sdkConfig.collectionId = oldCollectionId;
}

void AppcommServer::addChannelMember(const QString &channelId, const QString &userId) {
    if (channelId.isEmpty() || userId.isEmpty()) {
        emit membershipError(400, "Channel ID and User ID cannot be empty");
        return;
    }
    model::ChannelMember member;
    member.channelId = channelId;
    member.userId = userId;
    member.joinedAt = QDateTime::currentDateTimeUtc();
    member.lastSeenAt = QDateTime::currentDateTimeUtc();
    member.isActive = true;
    auto oldCollectionId = d->sdkConfig.collectionId;
    d->sdkConfig.collectionId = d->config.membersCollectionId;
    d->server->createDocument(d->sdkConfig, member.toJson());
    d->sdkConfig.collectionId = oldCollectionId;
    emit memberAdded(member);
}

void AppcommServer::removeChannelMember(const QString &channelId, const QString &userId) {
    if (channelId.isEmpty() || userId.isEmpty()) {
        emit membershipError(400, "Channel ID and User ID cannot be empty");
        return;
    }
    m_removingChannelId = channelId;
    m_removingUserId = userId;
    m_originalCollectionId = d->sdkConfig.collectionId;
    QJsonArray queries;
    queries.append(equalQuery("channelId", channelId));
    queries.append(equalQuery("userId", userId));
    d->sdkConfig.collectionId = d->config.membersCollectionId;
    disconnect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onServerRequestSuccess);
    connect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onMemberDocumentsListed);
    d->server->listDocuments(d->sdkConfig, queries);
}

void AppcommServer::getChannelMembers(const QString &channelId) {
    if (channelId.isEmpty()) {
        emit membershipError(400, "Channel ID cannot be empty");
        return;
    }
    m_queryingChannelId = channelId;
    m_originalCollectionId = d->sdkConfig.collectionId;
    QJsonArray queries;
    queries.append(equalQuery("channelId", channelId));
    d->sdkConfig.collectionId = d->config.membersCollectionId;
    disconnect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onServerRequestSuccess);
    connect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onMemberListingComplete);
    d->server->listDocuments(d->sdkConfig, queries);
}

void AppcommServer::onMemberDocumentsListed(const QJsonObject &data) {
    QJsonArray docs = data.value("documents").toArray();
    if (!docs.isEmpty()) {
        QString docId = docs.first().toObject().value("$id").toString();
        if (!docId.isEmpty()) {
            d->server->deleteDocument(d->sdkConfig, docId);
        }
    }
    disconnect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onMemberDocumentsListed);
    connect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onServerRequestSuccess);
    d->sdkConfig.collectionId = m_originalCollectionId;
    emit memberRemoved(m_removingChannelId, m_removingUserId);
}

void AppcommServer::onMemberListingComplete(const QJsonObject &data) {
    QJsonArray docs = data.value("documents").toArray();
    QList<model::ChannelMember> members;
    for (const QJsonValue &docValue : docs) {
        model::ChannelMember member = model::ChannelMember::fromJson(docValue.toObject());
        if (!member.userId.isEmpty()) {
            members.append(member);
        }
    }
    disconnect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onMemberListingComplete);
    connect(d->server, &appwritesdk::Server::requestSuccess, this, &AppcommServer::onServerRequestSuccess);
    d->sdkConfig.collectionId = m_originalCollectionId;
    emit membersListed(m_queryingChannelId, members);
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

    // User creation
    if (data.contains("email") && data.contains("$id")) {
        model::User user;
        user.userId = data.value("$id").toString();
        user.email = data.value("email").toString();
        d->users[user.userId] = user;
        emit userCreated(user);
        return;
    }

    // User listing
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

    // Messages listing
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
    if (!d->initialized) {
        emit initializationError(code, message);
    } else {
        emit operationError(code, message);
    }
}
