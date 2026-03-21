/*!
 * @file recentmessagecache.h
 * @brief LRU cache for recent messages with recovery support
 *
 * Maintains a configurable-capacity cache of recent messages
 * to support message recovery and offline resilience.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#ifndef RECENTMESSAGECACHE_H
#define RECENTMESSAGECACHE_H

#include <QObject>
#include <QString>
#include <QQueue>
#include <QHash>

#include "model.h"

namespace appcomm {

/*!
 * @brief LRU cache for recent messages.
 *
 * Stores the most recent N messages with efficient lookup
 * and supports recovery from a specific message ID.
 */
class RecentMessageCache : public QObject
{
    Q_OBJECT
public:

    /*!
     * @brief Constructs a RecentMessageCache instance.
     *
     * @param capacity Maximum number of messages to store (default 100).
     * @param parent Parent QObject for memory management.
     */
    explicit RecentMessageCache(int capacity = 100, QObject *parent = nullptr);

    /*!
     * @brief Adds a message to the cache.
     *
     * If cache is at capacity, removes the oldest message.
     * If messageId already exists, updates the existing entry.
     *
     * @param msg Message to add to cache.
     */
    void addMessage(const model::Message &msg);

    /*!
     * @brief Retrieves messages after a specific message ID.
     *
     * Returns all messages that came after the specified messageId,
     * in chronological order.
     *
     * @param fromMessageId Starting point (exclusive).
     * @return List of messages after fromMessageId, empty if not found.
     */
    QList<model::Message> getMessagesSince(const QString &messageId) const;

    /*!
     * @brief Gets a specific message by ID.
     *
     * @param messageId Message identifier to retrieve.
     * @return Message if found, invalid Message otherwise.
     */
    model::Message getMessage(const QString &messageId) const;

    /*!
     * @brief Checks if a message exists in the cache.
     *
     * @param messageId Message identifier to check.
     * @return true if message exists, false otherwise.
     */
    bool contains(const QString &messageId) const;

    /*!
     * @brief Gets all cached messages in chronological order.
     *
     * @return List of all cached messages.
     */
    QList<model::Message> getAllMessages() const;

    /*!
     * @brief Clears all messages from the cache.
     */
    void clear();

    /*!
     * @brief Gets the current number of cached messages.
     *
     * @return Number of messages in cache.
     */
    inline int size() const { return m_queue.size(); }

    /*!
     * @brief Gets the maximum cache capacity.
     *
     * @return Maximum number of messages.
     */
    inline int capacity() const { return m_capacity; }

    /*!
     * @brief Checks if the cache is empty.
     *
     * @return true if cache is empty, false otherwise.
     */
    inline bool isEmpty() const { return m_queue.isEmpty(); }

private:
    int m_capacity;                         ///< Maximum cache capacity
    QQueue<QString> m_queue;                ///< Message order (FIFO)
    QHash<QString, model::Message> m_cache; ///< Fast lookup by messageId
};

} // namespace appcomm

#endif // RECENTMESSAGECACHE_H