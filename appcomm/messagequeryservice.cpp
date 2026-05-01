/*!
 * @file messagequeryservice.cpp
 * @brief Implementation of Appwrite query builder helpers.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#include "messagequeryservice.h"

namespace appcomm {

namespace server {

MessageQueryService::MessageQueryService(QObject *parent)
    : QObject{parent}
{}

QString MessageQueryService::escapeQueryValue(const QString &value) {
    QString escaped = value;
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");
    return escaped;
}

QString MessageQueryService::equalQuery(const QString &key, const QString &value) {
    return QString("equal(\"%1\",[\"%2\"])").arg(key, escapeQueryValue(value));
}

QJsonArray MessageQueryService::topicMessages(const QString &topicId, int limit) const {
    QJsonArray queries;
    const QString trimmedTopicId = topicId.trimmed();
    if (trimmedTopicId.isEmpty()) {
        return queries;
    }
    queries.append(equalQuery("topicId", trimmedTopicId));
    queries.append("orderDesc(\"sequenceNumber\")");
    if (limit > 0) {
        queries.append(QString("limit(%1)").arg(limit));
    }
    return queries;
}

QJsonArray MessageQueryService::topicDocuments(const QString &topicId, int limit) const {
    QJsonArray queries;
    const QString trimmedTopicId = topicId.trimmed();
    if (trimmedTopicId.isEmpty()) {
        return queries;
    }
    queries.append(equalQuery("topicId", trimmedTopicId));
    if (limit > 0) {
        queries.append(QString("limit(%1)").arg(limit));
    }
    return queries;
}

QJsonArray MessageQueryService::topicMemberDocuments(const QString &topicId,
                                                       const QString &userId,
                                                       int limit) const {
    QJsonArray queries;
    const QString trimmedTopicId = topicId.trimmed();
    const QString trimmedUserId = userId.trimmed();
    if (trimmedTopicId.isEmpty() || trimmedUserId.isEmpty()) {
        return queries;
    }
    queries.append(equalQuery("topicId", trimmedTopicId));
    queries.append(equalQuery("userId", trimmedUserId));
    if (limit > 0) {
        queries.append(QString("limit(%1)").arg(limit));
    }
    return queries;
}

QJsonArray MessageQueryService::messageDocuments(const QString &messageId, int limit) const {
    QJsonArray queries;
    const QString trimmedMessageId = messageId.trimmed();
    if (trimmedMessageId.isEmpty()) {
        return queries;
    }
    queries.append(equalQuery("messageId", trimmedMessageId));
    if (limit > 0) {
        queries.append(QString("limit(%1)").arg(limit));
    }
    return queries;
}

QJsonArray MessageQueryService::lastSequenceForTopic(const QString &topicId) const {
    QJsonArray queries;
    const QString trimmedTopicId = topicId.trimmed();
    if (trimmedTopicId.isEmpty()) {
        return queries;
    }

    queries.append(equalQuery("topicId", trimmedTopicId));
    queries.append("orderDesc(\"sequenceNumber\")");
    queries.append("limit(1)");
    return queries;
}

} // namespace server

} // namespace appcomm