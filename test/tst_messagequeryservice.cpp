#include <QTest>

#include "messagequeryservice.h"

using namespace appcomm::server;

class tst_messagequeryservice : public QObject
{
    Q_OBJECT

private slots:
    void channelMessages_buildsChannelFilterOrderAndLimit();
    void channelDocuments_withoutLimit_buildsOnlyChannelFilter();
    void channelMemberDocuments_buildsChannelAndUserFilter();
    void messageDocuments_buildsMessageFilter();
    void equalQuery_escapesSpecialCharacters();
};

void tst_messagequeryservice::channelMessages_buildsChannelFilterOrderAndLimit() {
    MessageQueryService service;
    const QJsonArray queries = service.channelMessages("room-1", 25);

    QCOMPARE(queries.size(), 3);
    QCOMPARE(queries.at(0).toString(), QString("equal(\"channelId\",[\"room-1\"])"));
    QCOMPARE(queries.at(1).toString(), QString("orderDesc(\"sequenceNumber\")"));
    QCOMPARE(queries.at(2).toString(), QString("limit(25)"));
}

void tst_messagequeryservice::channelDocuments_withoutLimit_buildsOnlyChannelFilter() {
    MessageQueryService service;
    const QJsonArray queries = service.channelDocuments("room-1");

    QCOMPARE(queries.size(), 1);
    QCOMPARE(queries.at(0).toString(), QString("equal(\"channelId\",[\"room-1\"])"));
}

void tst_messagequeryservice::channelMemberDocuments_buildsChannelAndUserFilter() {
    MessageQueryService service;
    const QJsonArray queries = service.channelMemberDocuments("room-1", "user-42", 1);

    QCOMPARE(queries.size(), 3);
    QCOMPARE(queries.at(0).toString(), QString("equal(\"channelId\",[\"room-1\"])"));
    QCOMPARE(queries.at(1).toString(), QString("equal(\"userId\",[\"user-42\"])"));
    QCOMPARE(queries.at(2).toString(), QString("limit(1)"));
}

void tst_messagequeryservice::messageDocuments_buildsMessageFilter() {
    MessageQueryService service;
    const QJsonArray queries = service.messageDocuments("msg-7", 1);

    QCOMPARE(queries.size(), 2);
    QCOMPARE(queries.at(0).toString(), QString("equal(\"messageId\",[\"msg-7\"])"));
    QCOMPARE(queries.at(1).toString(), QString("limit(1)"));
}

void tst_messagequeryservice::equalQuery_escapesSpecialCharacters() {
    const QString raw = QString("a\\b\"c");
    const QString query = MessageQueryService::equalQuery("field", raw);
    QCOMPARE(query, QString("equal(\"field\",[\"a\\\\b\\\"c\"])"));
}

QTEST_MAIN(tst_messagequeryservice)
#include "tst_messagequeryservice.moc"
