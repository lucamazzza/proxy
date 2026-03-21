#include "recoverymanager.h"
#include <QJsonArray>
#include <QJsonDocument>

using namespace appcomm;

RecoveryManager::RecoveryManager(AppwriteSDK::Client *client,
                                 RecentMessageCache *cache,
                                 const AppwriteSDK::ConnectionConfig &config,
                                 QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_cache(cache)
    , m_config(config)
    , m_currentOperation(OperationType::None)
{
    connect(m_client, &AppwriteSDK::Client::requestSuccess,
            this, &RecoveryManager::onRequestSuccess);
    connect(m_client, &AppwriteSDK::Client::requestError,
            this, &RecoveryManager::onRequestError);
}

void RecoveryManager::requestFrom(const QString &messageId) {
    if (messageId.isEmpty()) {
        emit recoveryError(-1, "Invalid messageId: cannot be empty");
        return;
    }
    QList<model::Message> cachedMessages = m_cache->getMessagesSince(messageId);
    if (!cachedMessages.isEmpty()) {
        QJsonArray messages;
        for (const auto &msg : cachedMessages) {
            messages.append(msg.toJson());
        }
        emit messagesRecovered(messages);
        return;
    }
    model::Message startMsg = m_cache->getMessage(messageId);
    if (startMsg.messageId.isEmpty()) {
        emit recoveryError(-1, "Message not found in cache and cannot determine timestamp");
        return;
    }
    m_currentOperation = OperationType::RequestFrom;
    m_currentMessageId = messageId;
    QJsonArray queries;
    queries.append(QString("greaterThan(\"timestamp\",\"%1\")").arg(startMsg.timestamp.toString(Qt::ISODate)));
    queries.append("orderAsc(\"timestamp\")");
    queries.append(QString("limit(%1)").arg(m_cache.size()));
    m_client->listDocuments(m_config, queries);
}

void RecoveryManager::request(const QString &messageId) {
    if (messageId.isEmpty()) {
        emit recoveryError(-1, "Invalid messageId: cannot be empty");
        return;
    }
    if (m_cache->contains(messageId)) {
        model::Message msg = m_cache->getMessage(messageId);
        QJsonArray messages;
        messages.append(msg.toJson());
        emit messagesRecovered(messages);
        return;
    }
    m_currentOperation = OperationType::RequestSingle;
    m_currentMessageId = messageId;
    m_client->getDocument(m_config, messageId);
}

void RecoveryManager::requestFullResync() {
    m_currentOperation = OperationType::FullResync;
    m_currentMessageId.clear();
    m_cache->clear();
    QJsonArray queries;
    queries.append("orderAsc(\"timestamp\")");
    queries.append("limit(100)");
    m_client->listDocuments(m_config, queries);
}

void RecoveryManager::onRequestSuccess(const QJsonObject &data) {
    switch (m_currentOperation) {
        case OperationType::RequestFrom:
        case OperationType::FullResync: {
            QJsonArray documents = data.value("documents").toArray();
            processRecoveredMessages(documents);
            
            if (m_currentOperation == OperationType::FullResync) {
                emit resyncCompleted(documents.size());
            }
            emit messagesRecovered(documents);
            break;
        }
        case OperationType::RequestSingle: {
            QJsonArray messages;
            messages.append(data);
            processRecoveredMessages(messages);
            emit messagesRecovered(messages);
            break;
        }
        case OperationType::None:
            break;
    }
    m_currentOperation = OperationType::None;
    m_currentMessageId.clear();
}

void RecoveryManager::onRequestError(int code, const QString &message) {
    emit recoveryError(code, message);
    m_currentOperation = OperationType::None;
    m_currentMessageId.clear();
}

void RecoveryManager::processRecoveredMessages(const QJsonArray &messages) {
    for (const QJsonValue &msgValue : messages) {
        QJsonObject msgObj = msgValue.toObject();
        model::Message msg = model::Message::fromJson(msgObj);
        if (msg.isValid()) {
            m_cache->addMessage(msg);
        }
    }
}
