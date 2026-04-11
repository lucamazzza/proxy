#include <QTest>

#include "membershipservice.h"

using namespace appcomm::server;

class tst_membershipservice : public QObject
{
    Q_OBJECT

private slots:
    void createActiveMember_setsExpectedDefaults();
    void parseMembers_filtersInvalidEntries();
    void resolveDocumentId_extractsId();
};

void tst_membershipservice::createActiveMember_setsExpectedDefaults() {
    MembershipService service;
    const QDateTime joinedAt = QDateTime::currentDateTimeUtc();

    const appcomm::model::ChannelMember member =
        service.createActiveMember("  room-1  ", "  user-9  ", joinedAt);

    QCOMPARE(member.channelId, QString("room-1"));
    QCOMPARE(member.userId, QString("user-9"));
    QCOMPARE(member.joinedAt, joinedAt);
    QCOMPARE(member.lastSeenAt, joinedAt);
    QVERIFY(member.isActive);
}

void tst_membershipservice::parseMembers_filtersInvalidEntries() {
    MembershipService service;

    appcomm::model::ChannelMember validMember;
    validMember.channelId = "room-1";
    validMember.userId = "user-9";
    validMember.displayName = "User Nine";
    validMember.joinedAt = QDateTime::currentDateTimeUtc();
    validMember.lastSeenAt = validMember.joinedAt;
    validMember.isActive = true;

    const QJsonObject validDoc = validMember.toJson();
    const QJsonObject invalidDoc = QJsonObject{{"channelId", "room-1"}};
    const QJsonArray docs = {validDoc, invalidDoc};

    const QList<appcomm::model::ChannelMember> members = service.parseMembers(docs);
    QCOMPARE(members.size(), 1);
    QCOMPARE(members.first().channelId, QString("room-1"));
    QCOMPARE(members.first().userId, QString("user-9"));
}

void tst_membershipservice::resolveDocumentId_extractsId() {
    MembershipService service;
    const QJsonObject doc = {
        {"$id", "doc-123"},
        {"channelId", "room-1"}
    };

    QCOMPARE(service.resolveDocumentId(doc), QString("doc-123"));
}

QTEST_MAIN(tst_membershipservice)
#include "tst_membershipservice.moc"
