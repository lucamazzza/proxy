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

appcomm::model::ChannelMember MembershipService::createActiveMember(const QString &channelId,
                                                                    const QString &userId,
                                                                    const QDateTime &joinedAt) const {
    appcomm::model::ChannelMember member;
    member.channelId = channelId.trimmed();
    member.userId = userId.trimmed();
    member.joinedAt = joinedAt;
    member.lastSeenAt = joinedAt;
    member.isActive = true;
    return member;
}

QList<appcomm::model::ChannelMember> MembershipService::parseMembers(const QJsonArray &documents) const {
    QList<appcomm::model::ChannelMember> members;
    for (const QJsonValue &docValue : documents) {
        if (!docValue.isObject()) {
            continue;
        }
        const appcomm::model::ChannelMember member =
            appcomm::model::ChannelMember::fromJson(docValue.toObject());
        if (!member.userId.trimmed().isEmpty() && !member.channelId.trimmed().isEmpty()) {
            members.append(member);
        }
    }
    return members;
}

QString MembershipService::resolveDocumentId(const QJsonObject &document) const {
    return document.value("$id").toString().trimmed();
}
