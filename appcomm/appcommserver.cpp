#include "appcommserver.h"
#include "appwritesdk.h"
#include "membershipservice.h"
#include "realtime.h"
#include "model.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QSet>

using namespace appcomm::server;

class AppcommServer::Private {
public:
    model::AppCommConfig config;
    appwritesdk::ConnectionConfig sdkConfig;
    QNetworkAccessManager *networkManager;
    appwritesdk::Server *server;
    MembershipService *membershipService;
    Realtime *realtime;
    bool initialized;
    QHash<QString, model::Channel> channels;
    QHash<QString, model::User> users;
    QSet<QString> createdAttributes;
    int expectedAttributeCount = 7;
    QStringList docsToDeleteInChannel;

    Private()
        : networkManager(nullptr)
        , server(nullptr)
        , membershipService(nullptr)
        , realtime(nullptr)
        , initialized(false)
    {}

    ~Private() {
        delete realtime;
        delete membershipService;
        delete server;
        delete networkManager;
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
        if (event.contains("payload")) {
            QJsonObject payloadObj = event.value("payload").toObject();
            model::Message msg = model::Message::fromJson(payloadObj);
            if (msg.isValid()) {
                emit messageBroadcasted(msg);
            }
        }
    });
    d->realtime->connect(channels);
    
    emit configured();
}

void AppcommServer::initialize() {
    d->server->createDatabase(d->sdkConfig, "appcomm");
}

void AppcommServer::setupCollection() {
    QJsonArray permissions;
    permissions.append("read(\"any\")");
    permissions.append("create(\"any\")");
    d->server->createCollection(d->sdkConfig, "messages", permissions);
}

void AppcommServer::createMessageAttributes() {
    d->server->createAttribute(d->sdkConfig, "string",   "channelId",      APPCOMM_ATTR_REQUIRED, QJsonObject{{"size", 128}});
    d->server->createAttribute(d->sdkConfig, "string",   "senderId",       APPCOMM_ATTR_REQUIRED, QJsonObject{{"size", 128}});
    d->server->createAttribute(d->sdkConfig, "string",   "messageId",      APPCOMM_ATTR_REQUIRED, QJsonObject{{"size", 128}});
    d->server->createAttribute(d->sdkConfig, "integer",  "sequenceNumber", APPCOMM_ATTR_REQUIRED, QJsonObject{{"min", 0}});
    d->server->createAttribute(d->sdkConfig, "datetime", "timestamp",      APPCOMM_ATTR_REQUIRED);
    d->server->createAttribute(d->sdkConfig, "string",   "payload",        APPCOMM_ATTR_REQUIRED, QJsonObject{{"size", 10000}});
    d->server->createAttribute(d->sdkConfig, "boolean",  "isEcho",         APPCOMM_ATTR_REQUIRED);
}

void AppcommServer::createIndexes() {
    d->server->createIndex(d->sdkConfig, "idx_channel",           "key",    {"channelId"});
    d->server->createIndex(d->sdkConfig, "idx_timestamp",         "key",    {"timestamp"});
    d->server->createIndex(d->sdkConfig, "idx_sequence",          "key",    {"sequenceNumber"});
    d->server->createIndex(d->sdkConfig, "idx_message_unique",    "unique", {"messageId"});
    d->server->createIndex(d->sdkConfig, "idx_channel_timestamp", "key",    {"channelId", "timestamp"});
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
    queries.append(QString("equal(\"channelId\",\"%1\")").arg(channelId));
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
    queries.append(QString("equal(\"channelId\",\"%1\")").arg(channelId));
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
    queries.append(QString("equal(\"channelId\",\"%1\")").arg(channelId));
    queries.append(QString("equal(\"userId\",\"%1\")").arg(userId));
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
    queries.append(QString("equal(\"channelId\",\"%1\")").arg(channelId));
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

void AppcommServer::onServerRequestSuccess(const QJsonObject &data) {
    // DB Creation
    if (data.contains("$id") && data.contains("name")) {
        QString name = data.value("name").toString();
        if (name.contains("appcomm") || name.contains("db")) {
            setupCollection();
            return;
        }
    }

    // Collection Creation
    if (data.contains("$id") && data.contains("$permissions")) {
        createMessageAttributes();
        return;
    }

    // Attribute creation
    if (data.contains("key") && data.contains("type")) {
        QString attrKey = data.value("key").toString();
        d->createdAttributes.insert(attrKey);
        if (d->createdAttributes.size() >= d->expectedAttributeCount) {
            createIndexes();
        }
        return;
    }

    // Index creation
    if (data.contains("key") && data.contains("status")) {
        if (!d->initialized) {
            d->initialized = true;
            emit initialized();
        }
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
    if (!d->initialized) {
        emit initializationError(code, message);
    } else {
        emit operationError(code, message);
    }
}