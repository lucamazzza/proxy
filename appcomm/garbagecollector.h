/*!
 * @file garbagecollector.h
 * @brief Automatic cleanup service for old messages and expired data
 *
 * Provides TTL-based (Time-To-Live) message cleanup functionality.
 * Deletes messages older than a specified number of days to maintain
 * database hygiene and comply with data retention policies.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#ifndef GARBAGECOLLECTOR_H
#define GARBAGECOLLECTOR_H

#include <QObject>
#include <QString>

#include "appwritesdk.h"
#include "model.h"

namespace appcomm {

namespace server {

/*!
 * @brief Automated garbage collection service for message cleanup.
 *
 * The GarbageCollector handles TTL-based message deletion, removing messages
 * older than a configured number of days. It performs batch deletion to
 * efficiently handle large volumes of old messages while tracking progress
 * and reporting errors.
 *
 * Features:
 * - TTL-based message deletion
 * - Batch processing (100 messages at a time)
 * - Progress tracking with deletion count
 * - Error handling and reporting
 * - Async operation with signals
 *
 * @see model::PersistencePolicy for TTL configuration
 */
class GarbageCollector : public QObject {
    Q_OBJECT
public:

    /*!
     * @brief Constructs a GarbageCollector instance.
     *
     * Initializes the garbage collector with a server SDK instance and
     * connection configuration for accessing the messages collection.
     *
     * @param server Server SDK instance for admin operations
     * @param config Connection configuration (endpoint, database, collection)
     * @param parent Parent QObject for memory management
     */
    explicit GarbageCollector(appwritesdk::Server *server,
                              const appwritesdk::ConnectionConfig &config,
                              QObject *parent = nullptr);

    /*!
     * @brief Runs cleanup operation based on the provided policy.
     *
     * Initiates an async cleanup operation that deletes all messages older
     * than the TTL specified in the policy. Messages are deleted in batches
     * of 100 to avoid overwhelming the server.
     *
     * The operation proceeds as follows:
     * 1. Query for messages older than cutoff date (limit 100)
     * 2. Queue document IDs for deletion
     * 3. Delete documents one by one
     * 4. Repeat until no more old messages found
     * 5. Emit cleanupComplete with total count
     *
     * @param policy Persistence policy containing messageTTL (in days)
     *
     * @note If policy.messageTTL <= 0, cleanup is skipped and cleanupComplete(0) is emitted
     * @note Operation is asynchronous - listen to cleanupComplete/cleanupError signals
     *
     * @see cleanupComplete()
     * @see cleanupError()
     */
    void runCleanup(const model::PersistencePolicy &policy);

signals:

    /*!
     * @brief Emitted when cleanup operation completes successfully.
     *
     * @param deletedCount Total number of messages deleted during cleanup
     */
    void cleanupComplete(int deletedCount);

    /*!
     * @brief Emitted when cleanup operation encounters an error.
     *
     * @param error Human-readable error description including error code
     */
    void cleanupError(const QString &error);

private slots:

    /*!
     * @brief Handles response from document listing query.
     *
     * Internal slot that processes the list of old documents returned by
     * the server. On first call, queues document IDs for deletion and
     * starts the deletion process.
     *
     * @param data JSON response containing "documents" array with document metadata
     */
    void onDocumentsListed(const QJsonObject &data);

    /*!
     * @brief Handles response from document deletion.
     *
     * Internal slot called after each successful document deletion.
     * Increments the deletion counter and continues with the next
     * document in the queue. Emits cleanupComplete when queue is empty.
     *
     * @param data JSON response from deletion operation (unused)
     */
    void onDocumentDeleted(const QJsonObject &data);

    /*!
     * @brief Handles errors during cleanup operation.
     *
     * Internal slot called when document listing or deletion fails.
     * Formats the error message and emits cleanupError signal.
     *
     * @param code HTTP error code
     * @param message Error message from server
     */
    void onError(int code, const QString &message);

private:
    QJsonArray cleanupQueries() const;

    static constexpr int CLEANUP_BATCH_SIZE = 100; ///< Number of documents to delete per batch

    appwritesdk::Server *m_server;          ///< Server SDK for admin operations
    appwritesdk::ConnectionConfig m_config; ///< Connection config for messages collection
    QStringList m_docsToDelete;             ///< Queue of document IDs to delete
    int m_deletedCount;                     ///< Counter of deleted messages
    QString m_cutoffTimestamp;              ///< ISO timestamp used for the current cleanup run
    bool m_cleanupInProgress = false;       ///< True while a cleanup run is active
};

} // namespace server

} // namespace appcomm

#endif // GARBAGECOLLECTOR_H
