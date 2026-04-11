#ifndef MEMBERSHIPSERVICE_H
#define MEMBERSHIPSERVICE_H

#include <QObject>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>

#include "model.h"

namespace appcomm {

namespace server {


class MembershipService : public QObject {
    Q_OBJECT
public:
    explicit MembershipService(QObject *parent = nullptr);

    model::ChannelMember createActiveMember(const QString &channelId,
                                            const QString &userId,
                                            const QDateTime &joinedAt = QDateTime::currentDateTimeUtc()) const;
    QList<model::ChannelMember> parseMembers(const QJsonArray &documents) const;
    QString resolveDocumentId(const QJsonObject &document) const;

signals:
};

} // namespace server

} // namespace appcomm

#endif // MEMBERSHIPSERVICE_H
