/*!
 * @file appcommserver.h
 * @brief Server-side communication component for managing channels, users, and messages
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
 * @brief Server component for managing communication channels, users, and messages
 *
 * This class provides server-side functionality for creating and managing
 * communication channels, users, and message broadcasting. It interfaces
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

    // Channel Management

    /*!
     * @brief Creates a new communication channel
     * @param channel Channel information
     */
    void createChannel(const model::Channel &channel);

    /*!
     * @brief Deletes a channel and all its messages
     * @param channelId Channel identifier
     */
    void deleteChannel(const QString &channelId);

    /*!
     * @brief Lists all available channels
     */
    void listChannels();

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
     * @brief Broadcasts a message to all clients in a channel
     * @param message Message to broadcast
     */
    void broadcastMessage(const model::Message &message);

    /*!
     * @brief Retrieves messages from a channel
     * @param channelId Channel identifier
     * @param limit Maximum number of messages (default 16)
     */
    void getChannelMessages(const QString &channelId, int limit = 16);

    /*!
     * @brief Deletes a specific message
     * @param messageId Message identifier
     */
    void deleteMessage(const QString &messageId);

    // Membership Management

    /*!
     * @brief Adds a user to a channel
     * @param channelId Channel identifier
     * @param userId User identifier
     */
    void addChannelMember(const QString &channelId, const QString &userId);

    /*!
     * @brief Removes a user from a channel
     * @param channelId Channel identifier
     * @param userId User identifier
     */
    void removeChannelMember(const QString &channelId, const QString &userId);

    /*!
     * @brief Gets all members of a channel
     * @param channelId Channel identifier
     */
    void getChannelMembers(const QString &channelId);

    // State Queries

    /*!
     * @brief Checks if the server is initialized
     * @return true if initialized
     */
    bool isInitialized() const;

    /*!
     * @brief Gets all channels
     * @return List of channels
     */
    QList<model::Channel> channels() const;

    /*!
     * @brief Gets a specific channel
     * @param channelId Channel identifier
     * @return Channel or invalid channel if not found
     */
    model::Channel getChannel(const QString &channelId) const;

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

    // Channel signals

    /*!
     * @brief Emitted when a channel is created
     * @param channel Created channel
     */
    void channelCreated(const model::Channel &channel);

    /*!
     * @brief Emitted when a channel is deleted
     * @param channelId Deleted channel ID
     */
    void channelDeleted(const QString &channelId);

    /*!
     * @brief Emitted when channels are listed
     * @param channels List of channels
     */
    void channelsListed(const QList<model::Channel> &channels);

    /*!
     * @brief Emitted when a channel operation fails
     * @param code Error code
     * @param message Error message
     */
    void channelError(int code, const QString &message);

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
     * @brief Emitted when a member is added to a channel
     * @param member Added member
     */
    void memberAdded(const model::ChannelMember &member);

    /*!
     * @brief Emitted when a member is removed from a channel
     * @param channelId Channel ID
     * @param userId User ID
     */
    void memberRemoved(const QString &channelId, const QString &userId);

    /*!
     * @brief Emitted when channel members are listed
     * @param channelId Channel ID
     * @param members List of members
     */
    void membersListed(const QString &channelId,
                       const QList<model::ChannelMember> &members);

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
    QString m_deletingChannelId;
    QString m_originalCollectionId;
    QString m_removingChannelId;
    QString m_removingUserId;
    QString m_queryingChannelId;
};

} // namespace server

} // namespace appcomm

#endif // APPCOMMSERVER_H