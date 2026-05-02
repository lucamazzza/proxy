#ifndef BACKENDSERVICE_H
#define BACKENDSERVICE_H

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>

#include "appwritesdk.h"
#include "backendbootstrap.h"
#include "backendconfig.h"

namespace backend {

class Session {
public:
    QString sessionId;
    QString topicId;
    QStringList userIds;
    QDateTime createdAt;
    QString status = "open";
    bool isValid() const;
    QJsonObject toJson() const;
    static Session fromJson(const QJsonObject &obj);
};

class BackendService : public QObject {
    Q_OBJECT
public:
    explicit BackendService(const BackendConfig &config, QObject *parent = nullptr);
    BackendConfig resolvedConfig() const;
    bool bootstrap(QString *errorMessage);
    bool createTopic(const QString &name, QJsonObject *createdTopic, QString *errorMessage);
    bool createUser(const QString &email, const QString &password, const QString &name, QJsonObject *createdUser,
                    QString *errorMessage);
    bool deleteUser(const QString &userId, QString *errorMessage);
    bool listUsers(QJsonArray *users, QString *errorMessage);
    bool addCollection(const QString &collectionId, const QString &name, QJsonObject *createdCollection,
                       QString *errorMessage);
    bool removeCollection(const QString &collectionId, QString *errorMessage);
    bool listCollections(QJsonArray *collections, QString *errorMessage);
    bool listTopics(QJsonArray *topics, QString *errorMessage);
    bool removeTopic(const QString &topicId, QString *errorMessage);
    bool addMember(const QString &topicId, const QString &userId, const QString &displayName, QJsonObject *member,
                   QString *errorMessage);
    bool removeMember(const QString &topicId, const QString &userId, QString *errorMessage);
    bool listMembers(const QString &topicId, QJsonArray *members, QString *errorMessage);
    bool createSession(const QString &topicId, const QStringList &userIds, QJsonObject *createdSession,
                       QString *errorMessage);
    bool listSessions(QJsonArray *sessions, QString *errorMessage);
    bool closeSession(const QString &sessionId, QString *errorMessage);
    bool readMessages(const QString &topicId, const QString &messageId, int limit, QJsonArray *messages,
                      QString *errorMessage);
    bool createMessage(const QString &topicId, const QString &senderId, const QJsonObject &payload,
                       QJsonObject *createdMessage, QString *errorMessage);
    bool removeMessage(const QString &messageId, QString *errorMessage);
    int runEchoService(QString *errorMessage);

private:
    BackendRequestResult performRequest(const std::function<void()> &call, int timeoutMs = 30000);
    static QString makeUuid();
    static QString toError(const QString &operation, const BackendRequestResult &result);
    appwritesdk::ConnectionConfig
    configForCollection(const QString &collectionId) const;
    bool listDocuments(const QString &collectionId, const QJsonArray &queries, QJsonArray *documents,
                       QString *errorMessage);
    bool listAllDocuments(const QString &collectionId, QJsonArray *documents, QString *errorMessage);
    bool listAllUsers(QJsonArray *users, QString *errorMessage);
    bool createDocument(const QString &collectionId, const QJsonObject &data, QJsonObject *responseData,
                        QString *errorMessage);
    bool deleteDocument(const QString &collectionId, const QString &documentId, QString *errorMessage);
    bool findSingleDocument(const QString &collectionId, const QJsonArray &queries, QJsonObject *document,
                            QString *errorMessage);
    bool ensureTopicExists(const QString &topicId, QString *errorMessage);
    bool deleteDocumentsByTopic(const QString &collectionId, const QString &topicId, QString *errorMessage);
    BackendConfig m_config;
    QNetworkAccessManager m_network;
    appwritesdk::Server m_server;
};

} // namespace backend

#endif // BACKENDSERVICE_H
