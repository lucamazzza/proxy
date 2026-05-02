/*!
 * @file messagequeryservice.h
 * @brief Query builder helpers for Appwrite message/member document lookups.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#ifndef MESSAGEQUERYSERVICE_H
#define MESSAGEQUERYSERVICE_H

#include <QObject>
#include <QJsonArray>

namespace appcomm {

namespace server {

/*!
 * @brief Builds normalized Appwrite query arrays for server-side operations.
 *
 * This helper avoids repeating query string assembly logic in AppcommServer.
 */
class MessageQueryService : public QObject {
    Q_OBJECT
public:
    /*!
     * @brief Constructs the query service.
     * @param parent Optional Qt parent object.
     */
    explicit MessageQueryService(QObject *parent = nullptr);

    /*!
     * @brief Builds a query for listing topic messages by recency.
     * @param topicId Topic identifier.
     * @param limit Optional maximum number of messages.
     * @return Query array containing topic filter, descending timestamp order,
     *         and optional limit.
     */
    QJsonArray topicMessages(const QString &topicId, int limit) const;

    /*!
     * @brief Builds a query for topic-scoped documents.
     * @param topicId Topic identifier.
     * @param limit Optional maximum number of documents.
     * @return Query array containing topic filter and optional limit.
     */
    QJsonArray topicDocuments(const QString &topicId, int limit = -1) const;

    /*!
     * @brief Builds a query for topic-member documents.
     * @param topicId Topic identifier.
     * @param userId User identifier.
     * @param limit Optional maximum number of documents.
     * @return Query array containing topic/user filters and optional limit.
     */
    QJsonArray topicMemberDocuments(const QString &topicId,
                                      const QString &userId,
                                      int limit = 1) const;

    /*!
     * @brief Builds a query for message documents by logical messageId.
     * @param messageId Message identifier.
     * @param limit Optional maximum number of documents.
     * @return Query array containing message filter and optional limit.
     */
    QJsonArray messageDocuments(const QString &messageId, int limit = 1) const;
    QJsonArray lastSequenceForTopic(const QString &topicId) const;

    /*!
     * @brief Builds an Appwrite equal(...) query for a string field.
     * @param key Field key.
     * @param value Field value.
     * @return Escaped equal query string.
     */
    static QString equalQuery(const QString &key, const QString &value);

private:
    /*!
     * @brief Escapes a value for safe inclusion in Appwrite query syntax.
     * @param value Raw value.
     * @return Escaped value.
     */
    static QString escapeQueryValue(const QString &value);
};

} // namespace server

} // namespace appcomm

#endif // MESSAGEQUERYSERVICE_H
