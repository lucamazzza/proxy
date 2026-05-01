/*!
 * @file model.cpp
 * @brief Serialization, deserialization, and validation for AppComm models.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#include "model.h"

using namespace appcomm::model;

//User
bool User::isValid() const {
    return !userId.trimmed().isEmpty()
    && !email.trimmed().isEmpty(); //trimmed() rimuove spazi bianchi a inizio e fine stringa QT.
}

// Topic
bool Topic::isValid() const {
    return !topicId.trimmed().isEmpty()
    && !name.trimmed().isEmpty();
}

//Message
bool Message::isValid() const {
    return !topicId.trimmed().isEmpty()
    && !senderId.trimmed().isEmpty()
        && !messageId.trimmed().isEmpty()
        && timestamp.isValid();
}

QJsonObject Message::toJson() const {
    QJsonObject obj;
    obj["topicId"] = topicId;
    obj["senderId"] = senderId;
    obj["messageId"] = messageId;
    obj["sequenceNumber"] = sequenceNumber;
    obj["timestamp"] = timestamp.toString(Qt::ISODate);
    obj["payload"] = QString(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    obj["isEcho"] = isEcho;
    return obj;
}

Message Message::fromJson(const QJsonObject& obj) {
    Message message;

    const QJsonValue topicIdValue = obj.value("topicId");
    const QJsonValue senderIdValue = obj.value("senderId");
    const QJsonValue messageIdValue = obj.value("messageId");
    const QJsonValue sequenceNumberValue = obj.value("sequenceNumber");
    const QJsonValue timestampValue = obj.value("timestamp");
    const QJsonValue payloadValue = obj.value("payload");
    const QJsonValue isEchoValue = obj.value("isEcho");

    if (!topicIdValue.isString()
        || !senderIdValue.isString()
        || !messageIdValue.isString()
        || !timestampValue.isString()
        || !isEchoValue.isBool()) {
        return {};
    }

    message.topicId = topicIdValue.toString().trimmed();
    message.senderId = senderIdValue.toString().trimmed();
    message.messageId = messageIdValue.toString().trimmed();
    message.sequenceNumber = sequenceNumberValue.isDouble()
                                 ? sequenceNumberValue.toInteger(-1)
                                 : -1;
    message.timestamp = QDateTime::fromString(timestampValue.toString(), Qt::ISODate);

    if (payloadValue.isString()) {
        QJsonDocument doc = QJsonDocument::fromJson(payloadValue.toString().toUtf8());
        if (doc.isObject()) {
            message.payload = doc.object();
        }
    } else if (payloadValue.isObject()) {
        message.payload = payloadValue.toObject();
    }

    message.isEcho = isEchoValue.toBool();

    return message;
}

// PendingMessage
bool PendingMessage::isValid() const {
    return !topicId.trimmed().isEmpty()
    && !senderId.trimmed().isEmpty()
        && !messageId.trimmed().isEmpty()
        && timestamp.isValid();
}

QJsonObject PendingMessage::toJson() const {
    QJsonObject obj;
    obj["topicId"] = topicId;
    obj["senderId"] = senderId;
    obj["messageId"] = messageId;
    obj["timestamp"] = timestamp.toString(Qt::ISODate);
    obj["payload"] = QString(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    return obj;
}

PendingMessage PendingMessage::fromJson(const QJsonObject &obj) {
    PendingMessage message;

    const QJsonValue topicIdValue = obj.value("topicId");
    const QJsonValue senderIdValue = obj.value("senderId");
    const QJsonValue messageIdValue = obj.value("messageId");
    const QJsonValue timestampValue = obj.value("timestamp");
    const QJsonValue payloadValue = obj.value("payload");

    if (!topicIdValue.isString()
        || !senderIdValue.isString()
        || !messageIdValue.isString()
        || !timestampValue.isString()) {
        return {};
    }

    message.topicId = topicIdValue.toString().trimmed();
    message.senderId = senderIdValue.toString().trimmed();
    message.messageId = messageIdValue.toString().trimmed();
    message.timestamp = QDateTime::fromString(timestampValue.toString(), Qt::ISODate);

    if (payloadValue.isString()) {
        const QJsonDocument doc = QJsonDocument::fromJson(payloadValue.toString().toUtf8());
        if (doc.isObject()) {
            message.payload = doc.object();
        }
    } else if (payloadValue.isObject()) {
        message.payload = payloadValue.toObject();
    }

    return message;
}

//SessionInfo
bool SessionInfo::isExpired() const {
    return expiresAt.isValid()
    && expiresAt < QDateTime::currentDateTimeUtc();
}

SessionInfo SessionInfo::fromJson(const QJsonObject& obj) {
    SessionInfo sessionInfo;

    const QJsonValue userIdValue = obj.value("userId");
    const QJsonValue sessionIdValue = obj.value("sessionId");
    const QJsonValue authTypeValue = obj.value("authType");
    const QJsonValue createdAtValue = obj.value("createdAt");
    const QJsonValue expiresAtValue = obj.value("expiresAt");

    if (!userIdValue.isString()
        || !sessionIdValue.isString()
        || !authTypeValue.isDouble()
        || !createdAtValue.isString()
        || !expiresAtValue.isString()) {
        return {};
    }

    sessionInfo.userId = userIdValue.toString().trimmed();
    sessionInfo.sessionId = sessionIdValue.toString().trimmed();
    sessionInfo.authType = static_cast<AuthType>(authTypeValue.toInt());
    sessionInfo.createdAt = QDateTime::fromString(createdAtValue.toString(), Qt::ISODate);
    sessionInfo.expiresAt = QDateTime::fromString(expiresAtValue.toString(), Qt::ISODate);

    return sessionInfo;
}

//TopicMember
QJsonObject TopicMember::toJson() const {
    QJsonObject obj;
    obj["userId"] = userId;
    obj["topicId"] = topicId;
    obj["displayName"] = displayName;
    obj["joinedAt"] = joinedAt.toString(Qt::ISODate);
    obj["lastSeenAt"] = lastSeenAt.toString(Qt::ISODate);
    obj["isActive"] = isActive;
    return obj;
}

TopicMember TopicMember::fromJson(const QJsonObject& obj) {
    TopicMember member;

    const QJsonValue userIdValue = obj.value("userId");
    const QJsonValue topicIdValue = obj.value("topicId");
    const QJsonValue displayNameValue = obj.value("displayName");
    const QJsonValue joinedAtValue = obj.value("joinedAt");
    const QJsonValue lastSeenAtValue = obj.value("lastSeenAt");
    const QJsonValue isActiveValue = obj.value("isActive");

    if (!userIdValue.isString()
        || !topicIdValue.isString()
        || !displayNameValue.isString()
        || !joinedAtValue.isString()
        || !lastSeenAtValue.isString()
        || !isActiveValue.isBool()) {
        return {};
    }

    member.userId = userIdValue.toString().trimmed();
    member.topicId = topicIdValue.toString().trimmed();
    member.displayName = displayNameValue.toString().trimmed();
    member.joinedAt = QDateTime::fromString(joinedAtValue.toString(), Qt::ISODate);
    member.lastSeenAt = QDateTime::fromString(lastSeenAtValue.toString(), Qt::ISODate);
    member.isActive = isActiveValue.toBool();

    return member;
}

//AppCommConfig
bool AppCommConfig::isValid() const {
    return !endpoint.trimmed().isEmpty()
    && !projectId.trimmed().isEmpty()
        && !apiKey.trimmed().isEmpty()
        && !databaseId.trimmed().isEmpty()
        && !messagesCollectionId.trimmed().isEmpty()
        && !membersCollectionId.trimmed().isEmpty()
        && !incomingMessagesCollectionId.trimmed().isEmpty();
}
