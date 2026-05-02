/*!
 * @file appwritesdk.h
 * @brief Module in charge of the connection via HTTP to an Appwrite instance.
 *
 * Provides Qt-based wrapper classes for Appwrite REST API operations.
 * Includes client-side authentication and document operations, as well as
 * server-side administrative functions for database and user management.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#ifndef APPWRITESDK_H
#define APPWRITESDK_H

#define APPCOMM_USER false ///< `false` is mapped to the unprivileged mode
#define APPCOMM_ADMIN true ///< `true` is mapped to the privileged mode

#include <QtCore/qjsonarray.h>
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkCookieJar>

/*!
 * @brief Namespace for Appwrite SDK wrapper classes.
 *
 * Contains client and server SDK classes for interacting with Appwrite backend services.
 */
namespace appwritesdk {

/*!
 * @brief Configuration structure for Appwrite connections.
 *
 * Contains all necessary credentials and identifiers for connecting to
 * an Appwrite instance and accessing specific databases and collections.
 */
struct ConnectionConfig {
    QString endpoint;     ///< Appwrite server endpoint URL
    QString projectId;    ///< Project identifier
    QString apiKey;       ///< API key for server-side operations
    QString dbId;         ///< Database identifier
    QString collectionId; ///< Collection identifier
};

class IClientSdk {
public:
    virtual ~IClientSdk() = default;

    virtual void createAnonymousSession(const ConnectionConfig &config) = 0;

    virtual void createEmailSession(const ConnectionConfig &config,
                                    const QString &email,
                                    const QString &password) = 0;

    virtual void deleteSession(const ConnectionConfig &config,
                               const QString &sessionId) = 0;

    virtual void deleteSessions(const ConnectionConfig &config) = 0;

    virtual void getAccount(const ConnectionConfig &config) = 0;

    virtual void createDocument(const ConnectionConfig &config,
                                const QJsonObject &data) = 0;

    virtual void listDocuments(const ConnectionConfig &config,
                               const QJsonArray &queries = QJsonArray()) = 0;

    virtual void getDocument(const ConnectionConfig &config,
                             const QString &documentId) = 0;

    virtual void updateDocument(const ConnectionConfig &config,
                                const QString &documentId,
                                const QJsonObject &data) = 0;

    virtual void deleteDocument(const ConnectionConfig &config,
                                const QString &documentId) = 0;
};

/*!
 * @brief Base class for Appwrite SDK operations.
 *
 * Provides common functionality for HTTP requests, response handling,
 * and signal emission for both client and server SDK classes.
 */
class BaseSDK : public QObject {
    Q_OBJECT
public:
    /*!
     * @brief Constructs a BaseSDK instance.
     *
     * @param mgr Network access manager for handling HTTP requests
     * @param parent Parent QObject for memory management
     */
    explicit BaseSDK(QNetworkAccessManager *mgr, QObject *parent = nullptr);

signals:

    /*!
     * @brief Emitted when an HTTP request completes successfully.
     *
     * @param data JSON response data from the server
     */
    void requestSuccess(const QJsonObject &data);

    /*!
     * @brief Emitted when an HTTP request fails.
     *
     * @param code HTTP status code or error code
     * @param message Error message description
     */
    void requestError(int code, const QString &message);

protected slots:

    /*!
     * @brief Handles completion of network requests.
     *
     * Processes the HTTP response, parses JSON data, and emits
     * appropriate success or error signals.
     */
    void onResponseFinished();

protected:

    /*!
     * @brief Network access manager for HTTP operations.
     *
     * Shared pointer to the network manager used for all HTTP requests.
     */
    QNetworkAccessManager *m_network;

    /*!
     * @brief Creates a configured HTTP request with Appwrite headers.
     *
     * @param config Connection configuration containing endpoint and credentials
     * @param path API endpoint path (e.g., "/v1/account/sessions/anonymous")
     * @param isAdmin Whether to include API key for admin operations
     * @return Configured QNetworkRequest with appropriate headers
     */
    QNetworkRequest createBaseRequest(const ConnectionConfig &config, const QString &path, bool isAdmin);

    /*!
     * @brief Parses an error response from the HTTP endpoint.
     *
     * @param reply The reply received.
     */
    void parseErrorResponse(QNetworkReply *reply);
};

/*!
 * @brief Client-side Appwrite SDK for user authentication and data operations.
 *
 * Provides methods for creating user sessions and managing documents
 * without requiring administrative privileges.
 */
class Client : public BaseSDK, public IClientSdk {
    Q_OBJECT
public:
    using BaseSDK::BaseSDK;

    /*!
     * @brief Creates an anonymous user session.
     *
     * Establishes a session without requiring user credentials,
     * useful for guest access.
     *
     * @param config Connection configuration
     */
    void createAnonymousSession(const ConnectionConfig &config) override;

    /*!
     * @brief Creates a session using email and password authentication.
     *
     * @param config Connection configuration
     * @param email User email address
     * @param password User password
     */
    void createEmailSession(const ConnectionConfig &config, const QString &email, const QString &password) override;

    /*!
     * @brief Deletes a given session.
     *
     * @param config The config of the connection currently standing.
     * @param sessionId The ID of the session to destroy.
     */
    void deleteSession(const ConnectionConfig &config, const QString &sessionId) override;

    /*!
     * @brief Deletes all the sessions in the system (Broadcast logout).
     *
     * @param config The config of the connection currently standing.
     */
    void deleteSessions(const ConnectionConfig &config) override;

    /*!
     * @brief Retrieve the currently logged account.
     *
     * @param config The config of the connection currently standing.
     */
    void getAccount(const ConnectionConfig &config) override;

    /*!
     * @brief Creates a new document in the configured collection.
     *
     * @param config Connection configuration with database and collection IDs
     * @param data JSON object containing document data
     */
    void createDocument(const ConnectionConfig &config, const QJsonObject &data) override;

    /*!
     * @brief Lists documents in the configured collection with optional query filters.
     *
     * @param config Connection configuration with database and collection IDs
     * @param queries Optional array of query strings (e.g., ["equal(\"topicId\",\"general\")", "limit(100)"])
     */
    void listDocuments(const ConnectionConfig &config, const QJsonArray &queries = QJsonArray()) override;

    /*!
     * @brief Retrieves a single document by ID.
     *
     * @param config Connection configuration with database and collection IDs
     * @param documentId Unique document identifier
     */
    void getDocument(const ConnectionConfig &config, const QString &documentId) override;

    /*!
     * @brief Updates an existing document with new data.
     *
     * @param config Connection configuration with database and collection IDs
     * @param documentId Unique document identifier
     * @param data JSON object containing updated document data
     */
    void updateDocument(const ConnectionConfig &config, const QString &documentId, const QJsonObject &data) override;

    /*!
     * @brief Deletes a document from the collection.
     *
     * @param config Connection configuration with database and collection IDs
     * @param documentId Unique document identifier to delete
     */
    void deleteDocument(const ConnectionConfig &config, const QString &documentId) override;
};

/*!
 * @brief Server-side Appwrite SDK for administrative operations.
 *
 * Provides methods for managing databases, collections, attributes, and users.
 * Requires API key authentication for all operations.
 */
class Server : public BaseSDK {
    Q_OBJECT
public:
    using BaseSDK::BaseSDK;

    /*!
     * @brief Creates a new database in the Appwrite project.
     *
     * @param config Connection configuration with API key
     * @param name Database name
     */
    void createDatabase(const ConnectionConfig &config, const QString &name);

    /*!
     * @brief Deletes a database from the Appwrite project.
     *
     * @param config Connection configuration with database ID and API key
     */
    void deleteDatabase(const ConnectionConfig &config);

    /*!
     * @brief Lists databases in the current Appwrite project.
     *
     * @param config Connection configuration with API key
     */
    void listDatabases(const ConnectionConfig &config);

    /*!
     * @brief Creates a new collection in the specified database.
     *
     * @param config Connection configuration with database ID and API key
     * @param name Collection name
     * @param permissions Optional array of permission strings (default: read/create for anyone)
     */
    void createCollection(const ConnectionConfig &config, const QString &name, const QJsonArray &permissions = QJsonArray());

    /*!
     * @brief Deletes a collection from the database.
     *
     * @param config Connection configuration with database and collection IDs
     */
    void deleteCollection(const ConnectionConfig &config);

    /*!
     * @brief Updates the permissions for an existing collection.
     *
     * @param config Connection configuration with database and collection IDs
     * @param permissions Array of permission strings (e.g., ["read(\"any\")", "write(\"users\")"])
     */
    void updateCollectionPermissions(const ConnectionConfig &config, const QJsonArray &permissions);

    /*!
     * @brief Creates an attribute in the specified collection.
     *
     * Generic attribute creator that handles all types (string, integer, boolean, datetime).
     *
     * @param config Connection configuration with database and collection IDs
     * @param type Attribute type ("string", "integer", "boolean", "datetime")
     * @param key Attribute key/name
     * @param required Whether the attribute is required
     * @param options Additional options (e.g., {"size": 255} for strings, {"min": 0, "max": 100} for integers)
     */
    void createAttribute(const ConnectionConfig &config, 
                        const QString &type, 
                        const QString &key, 
                        bool required,
                        const QJsonObject &options = QJsonObject());

    /*!
     * @brief Creates an index on collection attributes.
     *
     * @param config Connection configuration with database and collection IDs
     * @param key Index key/name
     * @param type Index type ("key", "unique", "fulltext")
     * @param attributes List of attribute names to include in the index
     */
    void createIndex(const ConnectionConfig &config,
                    const QString &key,
                    const QString &type,
                    const QStringList &attributes);

    /*!
     * @brief Creates a new user account.
     *
     * @param config Connection configuration with API key
     * @param email User email address
     * @param password User password
     * @param name Optional user display name
     */
    void createUser(const ConnectionConfig &config, 
                   const QString &email, 
                   const QString &password,
                   const QString &name = QString());

    /*!
     * @brief Deletes a user account.
     *
     * @param config Connection configuration with API key
     * @param userId User ID to delete
     */
    void deleteUser(const ConnectionConfig &config, const QString &userId);

    /*!
     * @brief Lists users with optional query filters.
     *
     * @param config Connection configuration with API key
     * @param queries Optional query filters (e.g., limit, offset, search)
     */
    void listUsers(const ConnectionConfig &config, const QJsonArray &queries = QJsonArray());

    /*!
     * @brief Lists collections in the configured database.
     *
     * @param config Connection configuration with database ID and API key
     */
    void listCollections(const ConnectionConfig &config);

    /*!
     * @brief Lists documents in a collection (admin operation).
     *
     * @param config Connection configuration with database and collection IDs
     * @param queries Optional query filters
     */
    void listDocuments(const ConnectionConfig &config, const QJsonArray &queries = QJsonArray());

    /*!
     * @brief Creates a document in a collection (admin operation).
     *
     * @param config Connection configuration with database and collection IDs
     * @param data JSON object containing document data
     */
    void createDocument(const ConnectionConfig &config, const QJsonObject &data);

    /*!
     * @brief Deletes a document from a collection (admin operation).
     *
     * @param config Connection configuration with database and collection IDs
     * @param documentId Document ID to delete
     */
    void deleteDocument(const ConnectionConfig &config, const QString &documentId);
};

} // namespace AppwriteSDK

#endif // APPWRITESDK_H
