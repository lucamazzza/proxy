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
     * @brief Builds a query for listing channel messages by recency.
     * @param channelId Channel identifier.
     * @param limit Optional maximum number of messages.
     * @return Query array containing channel filter, descending timestamp order,
     *         and optional limit.
     */
    QJsonArray channelMessages(const QString &channelId, int limit) const;

    /*!
     * @brief Builds a query for channel-scoped documents.
     * @param channelId Channel identifier.
     * @param limit Optional maximum number of documents.
     * @return Query array containing channel filter and optional limit.
     */
    QJsonArray channelDocuments(const QString &channelId, int limit = -1) const;

    /*!
     * @brief Builds a query for channel-member documents.
     * @param channelId Channel identifier.
     * @param userId User identifier.
     * @param limit Optional maximum number of documents.
     * @return Query array containing channel/user filters and optional limit.
     */
    QJsonArray channelMemberDocuments(const QString &channelId,
                                      const QString &userId,
                                      int limit = 1) const;

    /*!
     * @brief Builds a query for message documents by logical messageId.
     * @param messageId Message identifier.
     * @param limit Optional maximum number of documents.
     * @return Query array containing message filter and optional limit.
     */
    QJsonArray messageDocuments(const QString &messageId, int limit = 1) const;

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
