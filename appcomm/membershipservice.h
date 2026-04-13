/*!
 * @file membershipservice.h
 * @brief Utilities for channel membership mapping and normalization.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#ifndef MEMBERSHIPSERVICE_H
#define MEMBERSHIPSERVICE_H

#include <QObject>
#include <QDateTime>
#include <QList>
#include <QJsonArray>
#include <QJsonObject>

#include "model.h"

namespace appcomm {

namespace server {

/*!
 * @brief Helper service for translating membership payloads.
 *
 * This service centralizes utility behavior used by AppcommServer for
 * channel membership creation and parsing.
 */
class MembershipService : public QObject {
    Q_OBJECT
public:
    /*!
     * @brief Constructs the membership service.
     * @param parent Optional Qt parent object.
     */
    explicit MembershipService(QObject *parent = nullptr);

    /*!
     * @brief Creates a normalized active member model.
     * @param channelId Channel identifier.
     * @param userId User identifier.
     * @param joinedAt Join timestamp used for both joinedAt and lastSeenAt.
     * @return A ChannelMember marked as active.
     */
    model::ChannelMember createActiveMember(const QString &channelId,
                                            const QString &userId,
                                            const QDateTime &joinedAt = QDateTime::currentDateTimeUtc()) const;

    /*!
     * @brief Parses and filters channel-member documents.
     * @param documents Raw Appwrite documents.
     * @return Valid parsed members with non-empty user and channel identifiers.
     */
    QList<model::ChannelMember> parseMembers(const QJsonArray &documents) const;

    /*!
     * @brief Resolves the Appwrite document identifier from a document object.
     * @param document Raw Appwrite document.
     * @return Trimmed document ID or an empty string if unavailable.
     */
    QString resolveDocumentId(const QJsonObject &document) const;
};

} // namespace server

} // namespace appcomm

#endif // MEMBERSHIPSERVICE_H
