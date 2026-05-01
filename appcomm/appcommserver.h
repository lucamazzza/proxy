/*!
 * @file appcommserver.h
 * @brief Server-side communication component for managing topics, users, and messages
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#ifndef APPCOMMSERVER_H
#define APPCOMMSERVER_H

#include <QObject>

#include "model.h"

#define APPCOMM_ATTR_REQUIRED true
#define APPCOMM_ATTR_OPTIONAL false

/*!
 * @namespace appcomm
 * @brief Main namespace for the application communication library
 */
namespace appcomm {

/*!
 * @namespace appcomm::server
 * @brief Server-side components for managing communication infrastructure
 */
namespace server {

/*!
 * @class AppcommServer
 * @brief Server component for managing communication topics, users, and messages
 *
 * This class provides server-side functionality for creating and managing
 * communication topics, users, and message broadcasting. It interfaces
 * with Appwrite backend services to provide persistent storage and real-time
 * communication capabilities.
 */
class AppcommServer : public QObject {
    Q_OBJECT
public:
    explicit AppcommServer(QObject *parent = nullptr);
    ~AppcommServer();

    /*!
     * @brief Configures the server with Appwrite connection details
     * @param config Configuration containing endpoint, API key, etc.
     */
    void configure(const model::AppCommConfig &config);

    /*!
     * @brief Initializes the server (creates database, collections, etc.)
     */
    void initialize();

    /*!
     * @brief Sets up the messages collection with proper permissions
     */
    void setupCollection();

    /*!
     * @brief Sets up the messages collection with proper permissions
     */
    void createMessageAttributes();

    /*!
     * @brief Creates database indexes for efficient querying
     */
    void createIndexes();

    // Topic Management

    /*!
     * @brief Creates a new communication topic
     * @param topic Topic information
     */
    void createTopic(const model::Topic &topic);

    /*!
     * @brief Deletes a topic and all its messages
     * @param topicId Topic identifier
     */
    void deleteTopic(const QString &topicId);

    /*!
     * @brief Lists all available topics
     */
    void listTopics();

    // User Management

    /*!
     * @brief Creates a new user account
     * @param email User email
     * @param password User password
     * @param name Optional display name
     */
    void createUser(const QString &email, const QString &password, const QString &name = QString());

    /*!
     * @brief Deletes a user account
     * @param userId User identifier
     */
    void deleteUser(const QString &userId);

    /*!
     * @brief Lists all users
     */
    void listUsers();

    // Message Management

    /*!
     * @brief Broadcasts a message to all clients in a topic
     * @param message Message to broadcast
     */
    void broadcastMessage(const model::Message &message);

    /*!
     * @brief Retrieves messages from a topic
     * @param topicId Topic identifier
     * @param limit Maximum number of messages (default 16)
     */
    void getTopicMessages(const QString &topicId, int limit = 16);

    /*!
     * @brief Deletes a specific message
     * @param messageId Message identifier
     */
    void deleteMessage(const QString &messageId);

    // Membership Management

    /*!
     * @brief Adds a user to a topic
     * @param topicId Topic identifier
     * @param userId User identifier
     */
    void addTopicMember(const QString &topicId, const QString &userId);

    /*!
     * @brief Removes a user from a topic
     * @param topicId Topic identifier
     * @param userId User identifier
     */
    void removeTopicMember(const QString &topicId, const QString &userId);

    /*!
     * @brief Gets all members of a topic
     * @param topicId Topic identifier
     */
    void getTopicMembers(const QString &topicId);

    // State Queries

    /*!
     * @brief Checks if the server is initialized
     * @return true if initialized
     */
    bool isInitialized() const;

    /*!
     * @brief Gets all topics
     * @return List of topics
     */
    QList<model::Topic> topics() const;

    /*!
     * @brief Gets a specific topic
     * @param topicId Topic identifier
     * @return Topic or invalid topic if not found
     */
    model::Topic getTopic(const QString &topicId) const;

signals:

    /*!
     * @brief Emitted when server is configured
     */
    void configured();

    // Initialization signals

    /*!
     * @brief Emitted when server initialization completes
     */
    void initialized();

    /*!
     * @brief Emitted when initialization fails
     * @param code Error code
     * @param message Error message
     */
    void initializationError(int code, const QString &message);

    // Topic signals

    /*!
     * @brief Emitted when a topic is created
     * @param topic Created topic
     */
    void topicCreated(const model::Topic &topic);

    /*!
     * @brief Emitted when a topic is deleted
     * @param topicId Deleted topic ID
     */
    void topicDeleted(const QString &topicId);

    /*!
     * @brief Emitted when topics are listed
     * @param topics List of topics
     */
    void topicsListed(const QList<model::Topic> &topics);

    /*!
     * @brief Emitted when a topic operation fails
     * @param code Error code
     * @param message Error message
     */
    void topicError(int code, const QString &message);

    // User signals

    /*!
     * @brief Emitted when a user is created
     * @param user Created user
     */
    void userCreated(const model::User &user);

    /*!
     * @brief Emitted when a user is deleted
     * @param userId Deleted user ID
     */
    void userDeleted(const QString &userId);

    /*!
     * @brief Emitted when users are listed
     * @param users List of users
     */
    void usersListed(const QList<model::User> &users);

    /*!
     * @brief Emitted when a user operation fails
     * @param code Error code
     * @param message Error message
     */
    void userError(int code, const QString &message);

    // Message signals

    /*!
     * @brief Emitted when a message is broadcasted
     * @param message Broadcasted message
     */
    void messageBroadcasted(const model::Message &message);

    /*!
     * @brief Emitted when messages are retrieved
     * @param messages List of messages
     */
    void messagesRetrieved(const QList<model::Message> &messages);

    /*!
     * @brief Emitted when a message is deleted
     * @param messageId Deleted message ID
     */
    void messageDeleted(const QString &messageId);

    /*!
     * @brief Emitted when a message operation fails
     * @param code Error code
     * @param message Error message
     */
    void messageError(int code, const QString &message);

    // Membership signals

    /*!
     * @brief Emitted when a member is added to a topic
     * @param member Added member
     */
    void memberAdded(const model::TopicMember &member);

    /*!
     * @brief Emitted when a member is removed from a topic
     * @param topicId Topic ID
     * @param userId User ID
     */
    void memberRemoved(const QString &topicId, const QString &userId);

    /*!
     * @brief Emitted when topic members are listed
     * @param topicId Topic ID
     * @param members List of members
     */
    void membersListed(const QString &topicId,
                       const QList<model::TopicMember> &members);

    /*!
     * @brief Emitted when a membership operation fails
     * @param code Error code
     * @param message Error message
     */
    void membershipError(int code, const QString &message);

    // General signals

    /*!
     * @brief Emitted when any operation fails
     * @param code Error code
     * @param message Error message
     */
    void operationError(int code, const QString &message);


private slots:
    /*!
     * @brief Handles successful server request responses
     * @param data Response data from the server
     */
    void onServerRequestSuccess(const QJsonObject &data);

    /*!
     * @brief Handles server request errors
     * @param code Error code
     * @param message Error message
     */
    void onServerRequestError(int code, const QString &message);

private:
    bool handleOperationSuccess(const QJsonObject &data);
    bool handleOperationError(int code, const QString &message);
    bool handleInitializationSuccess(const QJsonObject &data);
    bool handleInitializationError(int code, const QString &message);
    void continueInitializationAfterDatabase();
    void requestNextBootstrapAttribute();
    void requestNextBootstrapIndex();
    void requestCurrentBootstrapIndex();
    void completeInitialization();
    void failInitialization(int code, const QString &message);

    void enqueueIncomingMessage(const QString &documentId, const model::PendingMessage &message);
    void processNextIncomingMessage();
    /*!
     * @class Private
     * @brief Private implementation class (PIMPL pattern)
     */
    class Private;
    Private *d;                     ///< Pointer to private implementation
    QString m_deletingTopicId;
    QString m_originalCollectionId;
    QString m_removingTopicId;
    QString m_removingUserId;
    QString m_queryingTopicId;
};

} // namespace server

} // namespace appcomm

#endif // APPCOMMSERVER_H
