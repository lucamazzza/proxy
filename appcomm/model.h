/*!
 * @file model.h
 * @brief Domain model definitions for the AppComm communication layer.
 *
 * Defines the core data structures exchanged between frontend, backend,
 * and proxy. These structures are designed to be lightweight, serializable,
 * and reusable across different modules.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#ifndef MODEL_H
#define MODEL_H

#include <QString>
#include <QJsonObject>
#include <QDateTime>

namespace appcomm {

/*!
 * @brief Holds the model definitions of data that is transfered through Appwrite.
 *
 * Contains model structure definitions of data commonly transferred through Appwrite
 * that concern the messages sent to the client and to the server.
 */
namespace model {

/*!
 * @brief Authentication type used to establish a session.
 *
 * Represents how a user is authenticated within the system.
 */
enum class AuthType {
    Guest = 0, ///< Anonymous session
    Email      ///< Email/password authentication
};

/*!
 * @brief Represents a user in the system.
 *
 * Minimal user representation used across communication layers.
 */
struct User {
    QString userId; ///< Unique user identifier
    QString email;  ///< User email address

    /*!
     * @brief Validates the user object.
     * @return True if all required fields are non-empty
     */
    bool isValid() const;
};

/*!
 * @brief Represents a communication channel.
 *
 * A channel is identified by a unique ID and optionally a human-readable name.
 */
struct Channel {
    QString channelId; ///< Unique channel identifier (UUID)
    QString name;      ///< Human-readable channel name

    /*!
     * @brief Validates the channel object.
     * @return True if all required fields are non-empty
     */
    bool isValid() const;
};

/*!
 * @brief Represents a message exchanged within a channel.
 *
 * Messages are serialized as JSON and can carry arbitrary payload data.
 */
struct Message {
    QString channelId;   ///< Channel to which the message belongs
    QString senderId;    ///< ID of the sender user
    QString messageId;   ///< Unique message identifier
    QDateTime timestamp; ///< Message creation timestamp (UTC)
    QJsonObject payload; ///< Arbitrary JSON payload
    bool isEcho = false; ///< True if message was echoed by server

    /*!
     * @brief Validates the message object.
     * @return True if required fields are valid and timestamp is valid
     */
    bool isValid() const;

    /*!
     * @brief Serializes the message to JSON format.
     * @return JSON representation of the message
     */
    QJsonObject toJson() const;

    /*!
     * @brief Deserializes a message from JSON.
     *
     * Returns a default (empty) message if validation fails.
     *
     * @param obj JSON object to parse
     * @return Parsed Message instance or empty object
     */
    static Message fromJson(const QJsonObject& obj);
};

/*!
 * @brief Represents session metadata for an authenticated user.
 *
 * Contains authentication type and validity period of the session.
 */
struct SessionInfo {
    QString userId;      ///< ID of the associated user
    QString sessionId;   ///< Unique session identifier
    AuthType authType;   ///< Authentication method used
    QDateTime createdAt; ///< Session creation timestamp (UTC)
    QDateTime expiresAt; ///< Session expiration timestamp (UTC)

    /*!
     * @brief Checks whether the session is expired.
     * @return True if expiration time is valid and in the past
     */
    bool isExpired() const;

    /*!
     * @brief Deserializes session info from JSON.
     *
     * Returns a default (empty) object if validation fails.
     *
     * @param obj JSON object to parse
     * @return Parsed SessionInfo or empty object
     */
    static SessionInfo fromJson(const QJsonObject& obj);
};

/*!
 * @brief Represents a member of a channel.
 *
 * Tracks user presence and activity within a channel.
 */
struct ChannelMember {
    QString userId;       ///< User identifier
    QString channelId;    ///< Channel identifier
    QString displayName;  ///< Display name within the channel
    QDateTime joinedAt;   ///< Join timestamp (UTC)
    QDateTime lastSeenAt; ///< Last activity timestamp (UTC)
    bool isActive;        ///< Whether the user is currently active

    /*!
     * @brief Serializes the channel member to JSON.
     * @return JSON representation of the member
     */
    QJsonObject toJson() const;

    /*!
     * @brief Deserializes a channel member from JSON.
     *
     * Returns a default (empty) object if validation fails.
     *
     * @param obj JSON object to parse
     * @return Parsed ChannelMember or empty object
     */
    static ChannelMember fromJson(const QJsonObject& obj);
};

/*!
 * @brief Defines persistence policies for messages and sessions.
 *
 * TTL values are expressed in seconds.
 * A value of -1 means no expiration.
 */
struct PersistencePolicy {
    int messageTTL = -1;          ///< Message time-to-live
    int sessionTTL = -1;          ///< Session time-to-live
    int inactiveChannelTTL = -1;  ///< Channel inactivity timeout
};

/*!
 * @brief Configuration for AppComm communication layer.
 *
 * Contains identifiers and credentials required to connect
 * to the Appwrite backend.
 */
struct AppCommConfig {
    QString endpoint;               ///< Appwrite endpoint URL
    QString projectId;              ///< Project identifier
    QString apiKey;                 ///< API key (server-side usage)
    QString databaseId;             ///< Database identifier
    QString messagesCollectionId;   ///< Messages collection ID
    QString membersCollectionId;    ///< Channel members collection ID

    /*!
     * @brief Validates the configuration.
     * @return True if all required fields are non-empty
     */
    bool isValid() const;
};

} // namespace model

} // namespace appcomm

#endif // MODEL_H
