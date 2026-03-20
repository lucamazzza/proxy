#ifndef MODEL_H
#define MODEL_H

#include <QString>
#include <QJsonObject>
#include <QDateTime>

namespace model {

enum class AuthType {
    Guest = 0,
    Email
};

struct User {
    QString userId;
    QString email;

    bool isValid() const;
};

struct Channel {
    QString channelId;
    QString name;

    bool isValid() const;
};

struct Message {
    QString channelId;
    QString senderId;
    QString messageId;
    QDateTime timestamp;
    QJsonObject payload;
    bool isEcho = false; //Distingue i messaggi originali da quelli ripubblicati dal server.

    bool isValid() const;
    QJsonObject toJson() const;
    static Message fromJson(const QJsonObject& obj);
};

struct SessionInfo {
    QString userId;
    QString sessionId;
    AuthType authType;
    QDateTime createdAt;
    QDateTime expiresAt;

    bool isExpired() const;
    static SessionInfo fromJson(const QJsonObject& obj);
};

struct ChannelMember {
    QString userId;
    QString channelId;
    QString displayName;
    QDateTime joinedAt;
    QDateTime lastSeenAt;
    bool isActive;

    QJsonObject toJson() const;
    static ChannelMember fromJson(const QJsonObject& obj);
};

struct PersistencePolicy {
    int messageTTL = -1; //Per non far contenere random memory, ma non metto logica di business in model.
    int sessionTTL = -1;
    int inactiveChannelTTL = -1;
};

struct AppCommConfig {
    QString endpoint;
    QString projectId;
    QString apiKey;
    QString databaseId;
    QString messagesCollectionId;
    QString membersCollectionId;

    bool isValid() const;
};

}

#endif // MODEL_H
