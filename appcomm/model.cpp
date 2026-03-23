#include "model.h"

using namespace appcomm::model;

//User
bool User::isValid() const {
    return !userId.trimmed().isEmpty()
    && !email.trimmed().isEmpty(); //trimmed() rimuove spazi bianchi a inizio e fine stringa QT.
}

//Channel
bool Channel::isValid() const {
    return !channelId.trimmed().isEmpty()
    && !name.trimmed().isEmpty();
}

//Message
bool Message::isValid() const {
    return !channelId.trimmed().isEmpty()
    && !senderId.trimmed().isEmpty()
        && !messageId.trimmed().isEmpty()
        && timestamp.isValid();
}

QJsonObject Message::toJson() const {
    QJsonObject obj;
    obj["channelId"] = channelId;
    obj["senderId"] = senderId;
    obj["messageId"] = messageId;
    obj["timestamp"] = timestamp.toString(Qt::ISODate);
    obj["payload"] = payload;
    obj["isEcho"] = isEcho;
    return obj;
}

Message Message::fromJson(const QJsonObject& obj) {
    Message message;

    const QJsonValue channelIdValue = obj.value("channelId");
    const QJsonValue senderIdValue = obj.value("senderId");
    const QJsonValue messageIdValue = obj.value("messageId");
    const QJsonValue timestampValue = obj.value("timestamp");
    const QJsonValue payloadValue = obj.value("payload");
    const QJsonValue isEchoValue = obj.value("isEcho");

    if (!channelIdValue.isString()
        || !senderIdValue.isString()
        || !messageIdValue.isString()
        || !timestampValue.isString()
        || !payloadValue.isObject()
        || !isEchoValue.isBool()) {
        return {};
    }

    message.channelId = channelIdValue.toString().trimmed();
    message.senderId = senderIdValue.toString().trimmed();
    message.messageId = messageIdValue.toString().trimmed();
    message.timestamp = QDateTime::fromString(timestampValue.toString(), Qt::ISODate);
    message.payload = payloadValue.toObject();
    message.isEcho = isEchoValue.toBool();

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

//ChannelMember
QJsonObject ChannelMember::toJson() const {
    QJsonObject obj;
    obj["userId"] = userId;
    obj["channelId"] = channelId;
    obj["displayName"] = displayName;
    obj["joinedAt"] = joinedAt.toString(Qt::ISODate);
    obj["lastSeenAt"] = lastSeenAt.toString(Qt::ISODate);
    obj["isActive"] = isActive;
    return obj;
}

ChannelMember ChannelMember::fromJson(const QJsonObject& obj) {
    ChannelMember member;

    const QJsonValue userIdValue = obj.value("userId");
    const QJsonValue channelIdValue = obj.value("channelId");
    const QJsonValue displayNameValue = obj.value("displayName");
    const QJsonValue joinedAtValue = obj.value("joinedAt");
    const QJsonValue lastSeenAtValue = obj.value("lastSeenAt");
    const QJsonValue isActiveValue = obj.value("isActive");

    if (!userIdValue.isString()
        || !channelIdValue.isString()
        || !displayNameValue.isString()
        || !joinedAtValue.isString()
        || !lastSeenAtValue.isString()
        || !isActiveValue.isBool()) {
        return {};
    }

    member.userId = userIdValue.toString().trimmed();
    member.channelId = channelIdValue.toString().trimmed();
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
        && !membersCollectionId.trimmed().isEmpty();
}
