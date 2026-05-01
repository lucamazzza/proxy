/*!
 * @file membershipservice.cpp
 * @brief Implementation of membership parsing and normalization helpers.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#include "membershipservice.h"

using namespace appcomm::server;

MembershipService::MembershipService(QObject *parent)
    : QObject{parent}
{}

appcomm::model::TopicMember MembershipService::createActiveMember(const QString &topicId,
                                                                    const QString &userId,
                                                                    const QDateTime &joinedAt) const {
    appcomm::model::TopicMember member;
    member.topicId = topicId.trimmed();
    member.userId = userId.trimmed();
    member.joinedAt = joinedAt;
    member.lastSeenAt = joinedAt;
    member.isActive = true;
    return member;
}

QList<appcomm::model::TopicMember> MembershipService::parseMembers(const QJsonArray &documents) const {
    QList<appcomm::model::TopicMember> members;
    for (const QJsonValue &docValue : documents) {
        if (!docValue.isObject()) {
            continue;
        }
        const appcomm::model::TopicMember member =
            appcomm::model::TopicMember::fromJson(docValue.toObject());
        if (!member.userId.trimmed().isEmpty() && !member.topicId.trimmed().isEmpty()) {
            members.append(member);
        }
    }
    return members;
}

QString MembershipService::resolveDocumentId(const QJsonObject &document) const {
    return document.value("$id").toString().trimmed();
}
