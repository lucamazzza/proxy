#include <QMetaObject>
#include <QSignalSpy>
#include <QTest>

#include "appcommclient.h"

using namespace appcomm::client;

class tst_appcommclient : public QObject
{
    Q_OBJECT

private slots:
    void requestSuccess_sessionPayload_authenticatesClient();
    void requestSuccess_nonSessionPayload_doesNotAuthenticateClient();
    void eventReceived_nestedPayload_emitsMessage();
    void eventReceived_missingPayload_isIgnored();

private:
    appwritesdk::ConnectionConfig makeConfig() const;
    appcomm::model::Message makeMessage(const QString &messageId, qint64 sequenceNumber) const;
};

appwritesdk::ConnectionConfig tst_appcommclient::makeConfig() const {
    appwritesdk::ConnectionConfig config;
    config.endpoint = "https://example.com/v1";
    config.projectId = "project-id";
    config.apiKey = "api-key";
    config.dbId = "database-id";
    config.collectionId = "messages";
    return config;
}

appcomm::model::Message tst_appcommclient::makeMessage(const QString &messageId,
                                                       qint64 sequenceNumber) const {
    appcomm::model::Message message;
    message.channelId = "room-1";
    message.senderId = "user-1";
    message.messageId = messageId;
    message.sequenceNumber = sequenceNumber;
    message.timestamp = QDateTime::currentDateTimeUtc();
    message.payload = QJsonObject{{"text", "hello"}};
    message.isEcho = false;
    return message;
}

void tst_appcommclient::requestSuccess_sessionPayload_authenticatesClient() {
    AppcommClient client(makeConfig());
    QSignalSpy authSpy(&client, &AppcommClient::authenticationStateChanged);

    const QJsonObject response = {
        {"$id", "session-1"},
        {"userId", "user-1"},
        {"provider", "email"},
        {"$createdAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"expire", QDateTime::currentDateTimeUtc().addSecs(3600).toString(Qt::ISODate)}
    };

    QVERIFY(QMetaObject::invokeMethod(&client,
                                      "onRequestSuccess",
                                      Qt::DirectConnection,
                                      Q_ARG(QJsonObject, response)));

    QCOMPARE(authSpy.count(), 1);
    QVERIFY(client.isAuthenticated());
    QCOMPARE(client.sessionInfo().sessionId, QString("session-1"));
    QCOMPARE(client.sessionInfo().userId, QString("user-1"));
    QCOMPARE(client.sessionInfo().authType, appcomm::model::AuthType::Email);
}

void tst_appcommclient::requestSuccess_nonSessionPayload_doesNotAuthenticateClient() {
    AppcommClient client(makeConfig());
    QSignalSpy authSpy(&client, &AppcommClient::authenticationStateChanged);

    const appcomm::model::Message message = makeMessage("msg-1", 0);
    const QJsonObject response = {
        {"$id", "document-1"},
        {"messageId", message.messageId},
        {"channelId", message.channelId},
        {"senderId", message.senderId},
        {"timestamp", message.timestamp.toString(Qt::ISODate)},
        {"payload", message.payload},
        {"isEcho", false}
    };

    QVERIFY(QMetaObject::invokeMethod(&client,
                                      "onRequestSuccess",
                                      Qt::DirectConnection,
                                      Q_ARG(QJsonObject, response)));

    QCOMPARE(authSpy.count(), 0);
    QVERIFY(!client.isAuthenticated());
}

void tst_appcommclient::eventReceived_nestedPayload_emitsMessage() {
    AppcommClient client(makeConfig());
    int receivedCount = 0;
    QObject::connect(&client,
                     &AppcommClient::messageReceived,
                     &client,
                     [&](const appcomm::model::Message &) {
                         receivedCount += 1;
                     });

    const appcomm::model::Message message = makeMessage("msg-2", 0);
    const QJsonObject event = {
        {"data", QJsonObject{{"payload", message.toJson()}}}
    };

    QVERIFY(QMetaObject::invokeMethod(&client,
                                      "onEventReceived",
                                      Qt::DirectConnection,
                                      Q_ARG(QJsonObject, event)));

    QCOMPARE(receivedCount, 1);
}

void tst_appcommclient::eventReceived_missingPayload_isIgnored() {
    AppcommClient client(makeConfig());
    int receivedCount = 0;
    QObject::connect(&client,
                     &AppcommClient::messageReceived,
                     &client,
                     [&](const appcomm::model::Message &) {
                         receivedCount += 1;
                     });

    const QJsonObject event = {
        {"events", QJsonArray{"databases.*.collections.*.documents.*"}}
    };

    QVERIFY(QMetaObject::invokeMethod(&client,
                                      "onEventReceived",
                                      Qt::DirectConnection,
                                      Q_ARG(QJsonObject, event)));

    QCOMPARE(receivedCount, 0);
}

QTEST_MAIN(tst_appcommclient)
#include "tst_appcommclient.moc"
