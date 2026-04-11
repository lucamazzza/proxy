#include <QtTest>
#include <QSignalSpy>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

#include "appcommclient.h"
#include "appwritesdk.h"
#include "model.h"
#include "realtime.h"
#include "recoverymanager.h"
#include "ratelimiter.h"

using namespace appcomm;
using namespace appcomm::client;

Q_DECLARE_METATYPE(appcomm::model::Message)

class FakeClientSdk : public QObject, public appwritesdk::IClientSdk
{
    Q_OBJECT

public:
    explicit FakeClientSdk(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    int createAnonymousSessionCalls = 0;
    int createEmailSessionCalls = 0;
    int deleteSessionCalls = 0;
    int createDocumentCalls = 0;

    QString lastEmail;
    QString lastPassword;
    QString lastDeletedSessionId;
    QJsonObject lastCreatedDocument;
    appwritesdk::ConnectionConfig lastConfig;

    void createAnonymousSession(const appwritesdk::ConnectionConfig &config) override
    {
        ++createAnonymousSessionCalls;
        lastConfig = config;
    }

    void createEmailSession(const appwritesdk::ConnectionConfig &config,
                            const QString &email,
                            const QString &password) override
    {
        ++createEmailSessionCalls;
        lastConfig = config;
        lastEmail = email;
        lastPassword = password;
    }

    void deleteSession(const appwritesdk::ConnectionConfig &config,
                       const QString &sessionId) override
    {
        ++deleteSessionCalls;
        lastConfig = config;
        lastDeletedSessionId = sessionId;
    }

    void createDocument(const appwritesdk::ConnectionConfig &config,
                        const QJsonObject &data) override
    {
        ++createDocumentCalls;
        lastConfig = config;
        lastCreatedDocument = data;
    }

signals:
    void requestSuccess(appwritesdk::RequestType type, const QJsonObject &data);
    void requestError(appwritesdk::RequestType type, int code, const QString &message);

public:
    void emitRequestSuccess(appwritesdk::RequestType type, const QJsonObject &data)
    {
        emit requestSuccess(type, data);
    }

    void emitRequestError(appwritesdk::RequestType type, int code, const QString &message)
    {
        emit requestError(type, code, message);
    }
};

class FakeRecoveryManager : public QObject, public IRecoveryManager
{
    Q_OBJECT

public:
    explicit FakeRecoveryManager(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    int requestFromCalls = 0;
    QString lastRequestedFrom;

    void requestFrom(const QString &messageId) override
    {
        ++requestFromCalls;
        lastRequestedFrom = messageId;
    }

signals:
    void messagesRecovered(const QJsonArray &messages);
    void recoveryError(int errorCode, const QString &errorMessage);
    void resyncCompleted(int messageCount);

public:
    void emitMessagesRecovered(const QJsonArray &messages)
    {
        emit messagesRecovered(messages);
    }

    void emitRecoveryError(int code, const QString &message)
    {
        emit recoveryError(code, message);
    }

    void emitResyncCompleted(int count)
    {
        emit resyncCompleted(count);
    }
};

class FakeRateLimiter : public IRateLimiter
{
public:
    bool allowNext = true;
    int allowRequestCalls = 0;
    int resetCalls = 0;

    bool allowRequest() override
    {
        ++allowRequestCalls;
        return allowNext;
    }

    void reset() override
    {
        ++resetCalls;
    }
};

class FakeRealtime : public appcomm::IRealtime
{
    Q_OBJECT

public:
    explicit FakeRealtime(QObject *parent = nullptr)
        : IRealtime(parent)
    {
    }

    int connectCalls = 0;
    int disconnectCalls = 0;
    QStringList lastChannels;

    void connectToChannels(const QStringList &channels) override
    {
        ++connectCalls;
        lastChannels = channels;
    }

    void disconnectFromServer() override
    {
        ++disconnectCalls;
    }

public:
    void emitConnected()
    {
        emit connected();
    }

    void emitDisconnected()
    {
        emit disconnected();
    }

    void emitEventReceived(const QJsonObject &event)
    {
        emit eventReceived(event);
    }

    void emitErrorOccurred(const QString &error)
    {
        emit errorOccurred(error);
    }
};

class tst_appcommclient : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void construction();
    void createGuestSession_callsClientSdk();
    void createEmailSession_callsClientSdk();

    void authSuccess_guest_updatesStateAndSignal();
    void authSuccess_email_updatesStateAndSignal();
    void requestError_emitsError();

    void joinChannel_empty_emitsError();
    void joinChannel_valid_setsActiveChannelAndEmitsSignal();
    void leaveChannel_clearsActiveChannelAndEmitsSignal();
    void leaveChannel_noActiveChannel_doesNothing();

    void connectToServer_notAuthenticated_doesNothing();
    void connectToServer_noChannel_doesNothing();
    void connectToServer_authenticatedAndChannel_connectsRealtime();
    void connectToServer_alreadyConnectingOrConnected_doesNothing();
    void onConnected_setsConnectedAndEmitsChannelReady();
    void disconnectFromServer_callsRealtimeAndChangesState();
    void disconnectFromServer_alreadyDisconnectedOrDisconnecting_doesNothing();
    void onDisconnected_setsDisconnected();

    void sendMessage_notAuthenticated_emitsError();
    void sendMessage_noChannel_emitsError();
    void sendMessage_rateLimited_emitsError();
    void sendMessage_valid_callsCreateDocument();

    void realtimeEvent_validMessage_emitsMessageReceived();
    void realtimeEvent_invalidPayload_doesNothing();
    void realtimeError_setsFailedAndEmitsError();

    void recoveryMessagesRecovered_emitsMessageReceived();
    void recoveryError_emitsError();

    void logout_authenticated_deletesSessionAndResetsState();
    void logout_notAuthenticated_doesNothing();

    void requestSuccess_nonAuthType_doesNothing();
    void authSuccess_whenAlreadyAuthenticated_doesNotEmitAgain();

private:
    appwritesdk::ConnectionConfig makeConfig() const;
    model::Message makeIncomingMessage(const QString &channelId,
                                       const QString &messageId,
                                       int sequence) const;

    FakeClientSdk *m_client = nullptr;
    FakeRealtime *m_realtime = nullptr;
    FakeRecoveryManager *m_recovery = nullptr;
    FakeRateLimiter *m_rateLimiter = nullptr;
    AppcommClient *m_app = nullptr;
};

appwritesdk::ConnectionConfig tst_appcommclient::makeConfig() const
{
    appwritesdk::ConnectionConfig config;
    config.endpoint = "http://localhost/v1";
    config.projectId = "test-project";
    config.apiKey = "test-api-key";
    config.dbId = "test-db";
    config.collectionId = "test-collection";
    return config;
}

model::Message tst_appcommclient::makeIncomingMessage(const QString &channelId,
                                                      const QString &messageId,
                                                      int sequence) const
{
    model::Message msg;
    msg.channelId = channelId;
    msg.senderId = "user-1";
    msg.messageId = messageId;
    msg.sequenceNumber = sequence;
    msg.timestamp = QDateTime::currentDateTimeUtc();
    msg.payload = QJsonObject{{"text", "hello"}};
    msg.isEcho = false;
    return msg;
}

void tst_appcommclient::init()
{
    qRegisterMetaType<appcomm::model::Message>("appcomm::model::Message");

    m_client = new FakeClientSdk(this);
    m_realtime = new FakeRealtime(this);
    m_recovery = new FakeRecoveryManager(this);
    m_rateLimiter = new FakeRateLimiter();

    m_app = new AppcommClient(makeConfig(),
                              m_client,
                              m_realtime,
                              m_recovery,
                              m_rateLimiter,
                              this);
}

void tst_appcommclient::cleanup()
{
    delete m_app;
    m_app = nullptr;

    delete m_rateLimiter;
    m_rateLimiter = nullptr;

    m_client = nullptr;
    m_realtime = nullptr;
    m_recovery = nullptr;
}

void tst_appcommclient::construction()
{
    QVERIFY(m_app != nullptr);
    QCOMPARE(m_app->connectionState(), ConnectionState::Disconnected);
    QCOMPARE(m_app->connectionStateText(), QString("Disconnected"));
    QVERIFY(!m_app->isAuthenticated());
    QCOMPARE(m_app->activeChannel(), QString());
}

void tst_appcommclient::createGuestSession_callsClientSdk()
{
    m_app->createGuestSession();

    QCOMPARE(m_client->createAnonymousSessionCalls, 1);
    QCOMPARE(m_client->lastConfig.projectId, QString("test-project"));
}

void tst_appcommclient::createEmailSession_callsClientSdk()
{
    m_app->createEmailSession("test@example.com", "secret");

    QCOMPARE(m_client->createEmailSessionCalls, 1);
    QCOMPARE(m_client->lastEmail, QString("test@example.com"));
    QCOMPARE(m_client->lastPassword, QString("secret"));
}

void tst_appcommclient::authSuccess_guest_updatesStateAndSignal()
{
    QSignalSpy spy(m_app, &AppcommClient::authenticationStateChanged);

    QJsonObject data{
        {"$id", "session-123"},
        {"userId", "user-123"}
    };

    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateAnonymousSession, data);

    QCOMPARE(spy.count(), 1);
    QVERIFY(m_app->isAuthenticated());
    QCOMPARE(m_app->sessionInfo().sessionId, QString("session-123"));
    QCOMPARE(m_app->sessionInfo().userId, QString("user-123"));
}

void tst_appcommclient::authSuccess_email_updatesStateAndSignal()
{
    QSignalSpy spy(m_app, &AppcommClient::authenticationStateChanged);

    QJsonObject data{
        {"$id", "session-456"},
        {"userId", "user-456"}
    };

    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateEmailSession, data);

    QCOMPARE(spy.count(), 1);
    QVERIFY(m_app->isAuthenticated());
    QCOMPARE(m_app->sessionInfo().sessionId, QString("session-456"));
    QCOMPARE(m_app->sessionInfo().userId, QString("user-456"));
}

void tst_appcommclient::requestError_emitsError()
{
    QSignalSpy spy(m_app, &AppcommClient::errorOccurred);

    m_client->emitRequestError(appwritesdk::RequestType::GetDocument, 500, "Boom");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("Boom"));
}

void tst_appcommclient::joinChannel_empty_emitsError()
{
    QSignalSpy spy(m_app, &AppcommClient::errorOccurred);

    m_app->joinChannel("   ");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("Channel ID cannot be empty."));
}

void tst_appcommclient::joinChannel_valid_setsActiveChannelAndEmitsSignal()
{
    QSignalSpy spy(m_app, &AppcommClient::activeChannelChanged);

    m_app->joinChannel("  general  ");

    QCOMPARE(m_app->activeChannel(), QString("general"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("general"));
}

void tst_appcommclient::leaveChannel_clearsActiveChannelAndEmitsSignal()
{
    m_app->joinChannel("general");
    QSignalSpy spy(m_app, &AppcommClient::leftChannel);

    m_app->leaveChannel();

    QCOMPARE(m_app->activeChannel(), QString());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("general"));
}

void tst_appcommclient::leaveChannel_noActiveChannel_doesNothing()
{
    QSignalSpy spy(m_app, &AppcommClient::leftChannel);

    m_app->leaveChannel();

    QCOMPARE(m_app->activeChannel(), QString());
    QCOMPARE(spy.count(), 0);
}

void tst_appcommclient::connectToServer_notAuthenticated_doesNothing()
{
    m_app->joinChannel("general");
    m_app->connectToServer();

    QCOMPARE(m_realtime->connectCalls, 0);
    QCOMPARE(m_app->connectionState(), ConnectionState::Disconnected);
}

void tst_appcommclient::connectToServer_noChannel_doesNothing()
{
    QJsonObject data{{"$id", "session-1"}, {"userId", "user-1"}};
    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateAnonymousSession, data);

    m_app->connectToServer();

    QCOMPARE(m_realtime->connectCalls, 0);
    QCOMPARE(m_app->connectionState(), ConnectionState::Disconnected);
}

void tst_appcommclient::connectToServer_authenticatedAndChannel_connectsRealtime()
{
    QJsonObject data{{"$id", "session-1"}, {"userId", "user-1"}};
    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateAnonymousSession, data);

    m_app->joinChannel("general");

    QCOMPARE(m_realtime->connectCalls, 1);
    QCOMPARE(m_app->connectionState(), ConnectionState::Connecting);
    QCOMPARE(m_realtime->lastChannels.size(), 1);
    QCOMPARE(m_realtime->lastChannels.first(),
             QString("databases.test-db.collections.test-collection.documents"));
}

void tst_appcommclient::connectToServer_alreadyConnectingOrConnected_doesNothing()
{
    QJsonObject data{{"$id", "session-1"}, {"userId", "user-1"}};
    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateAnonymousSession, data);

    m_app->joinChannel("general");

    int initialCalls = m_realtime->connectCalls;

    m_app->connectToServer();
    QCOMPARE(m_realtime->connectCalls, initialCalls);

    m_realtime->emitConnected();

    m_app->connectToServer();
    QCOMPARE(m_realtime->connectCalls, initialCalls);
}

void tst_appcommclient::onConnected_setsConnectedAndEmitsChannelReady()
{
    QJsonObject data{{"$id", "session-1"}, {"userId", "user-1"}};
    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateAnonymousSession, data);
    m_app->joinChannel("general");

    QSignalSpy stateSpy(m_app, &AppcommClient::connectionStateChanged);
    QSignalSpy readySpy(m_app, &AppcommClient::channelReady);

    m_realtime->emitConnected();

    QVERIFY(stateSpy.count() >= 1);
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(readySpy.at(0).at(0).toString(), QString("general"));
    QCOMPARE(m_app->connectionState(), ConnectionState::Connected);
    QCOMPARE(m_app->connectionStateText(), QString("Connected"));
}

void tst_appcommclient::disconnectFromServer_callsRealtimeAndChangesState()
{
    QJsonObject data{{"$id", "session-1"}, {"userId", "user-1"}};
    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateAnonymousSession, data);
    m_app->joinChannel("general");
    m_realtime->emitConnected();

    m_app->disconnectFromServer();

    QCOMPARE(m_realtime->disconnectCalls, 1);
    QCOMPARE(m_app->connectionState(), ConnectionState::Disconnecting);
}

void tst_appcommclient::disconnectFromServer_alreadyDisconnectedOrDisconnecting_doesNothing()
{
    m_app->disconnectFromServer();
    QCOMPARE(m_realtime->disconnectCalls, 0);

    QJsonObject data{{"$id", "session-1"}, {"userId", "user-1"}};
    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateAnonymousSession, data);
    m_app->joinChannel("general");
    m_realtime->emitConnected();

    m_app->disconnectFromServer();
    QCOMPARE(m_realtime->disconnectCalls, 1);

    m_app->disconnectFromServer();
    QCOMPARE(m_realtime->disconnectCalls, 1);
}

void tst_appcommclient::onDisconnected_setsDisconnected()
{
    QJsonObject data{{"$id", "session-1"}, {"userId", "user-1"}};
    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateAnonymousSession, data);
    m_app->joinChannel("general");
    m_realtime->emitConnected();

    m_realtime->emitDisconnected();

    QCOMPARE(m_app->connectionState(), ConnectionState::Disconnected);
    QCOMPARE(m_app->connectionStateText(), QString("Disconnected"));
}

void tst_appcommclient::sendMessage_notAuthenticated_emitsError()
{
    QSignalSpy spy(m_app, &AppcommClient::errorOccurred);

    m_app->sendMessage(QJsonObject{{"text", "hi"}});

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(),
             QString("Cannot send message: client is not authenticated."));
    QCOMPARE(m_client->createDocumentCalls, 0);
}

void tst_appcommclient::sendMessage_noChannel_emitsError()
{
    QJsonObject data{{"$id", "session-1"}, {"userId", "user-1"}};
    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateAnonymousSession, data);

    QSignalSpy spy(m_app, &AppcommClient::errorOccurred);

    m_app->sendMessage(QJsonObject{{"text", "hi"}});

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(),
             QString("Cannot send message: no active channel."));
    QCOMPARE(m_client->createDocumentCalls, 0);
}

void tst_appcommclient::sendMessage_rateLimited_emitsError()
{
    QJsonObject data{{"$id", "session-1"}, {"userId", "user-1"}};
    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateAnonymousSession, data);
    m_app->joinChannel("general");

    m_rateLimiter->allowNext = false;

    QSignalSpy spy(m_app, &AppcommClient::errorOccurred);

    m_app->sendMessage(QJsonObject{{"text", "hi"}});

    QCOMPARE(m_rateLimiter->allowRequestCalls, 1);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("Rate limit exceeded."));
    QCOMPARE(m_client->createDocumentCalls, 0);
}

void tst_appcommclient::sendMessage_valid_callsCreateDocument()
{
    QJsonObject data{{"$id", "session-1"}, {"userId", "user-42"}};
    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateAnonymousSession, data);
    m_app->joinChannel("general");

    m_rateLimiter->allowNext = true;

    QJsonObject payload{{"text", "hello"}};
    m_app->sendMessage(payload);

    QCOMPARE(m_rateLimiter->allowRequestCalls, 1);
    QCOMPARE(m_client->createDocumentCalls, 1);

    QCOMPARE(m_client->lastCreatedDocument.value("channelId").toString(), QString("general"));
    QCOMPARE(m_client->lastCreatedDocument.value("senderId").toString(), QString("user-42"));
    QCOMPARE(m_client->lastCreatedDocument.value("sequenceNumber").toInt(), -1);
    QVERIFY(!m_client->lastCreatedDocument.value("messageId").toString().isEmpty());

    const QString payloadString = m_client->lastCreatedDocument.value("payload").toString();
    QCOMPARE(payloadString, QString("{\"text\":\"hello\"}"));
}

void tst_appcommclient::realtimeEvent_validMessage_emitsMessageReceived()
{
    QJsonObject auth{{"$id", "session-1"}, {"userId", "user-1"}};
    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateAnonymousSession, auth);
    m_app->joinChannel("general");

    QSignalSpy spy(m_app, &AppcommClient::messageReceived);

    model::Message msg = makeIncomingMessage("general", "msg-1", 1);
    QJsonObject event{{"payload", msg.toJson()}};

    m_realtime->emitEventReceived(event);

    QCOMPARE(spy.count(), 1);
    const appcomm::model::Message received =
        qvariant_cast<appcomm::model::Message>(spy.at(0).at(0));
    QCOMPARE(received.messageId, QString("msg-1"));
    QCOMPARE(received.channelId, QString("general"));
}

void tst_appcommclient::realtimeEvent_invalidPayload_doesNothing()
{
    QSignalSpy spy(m_app, &AppcommClient::messageReceived);

    QJsonObject event{{"payload", "not-an-object"}};
    m_realtime->emitEventReceived(event);

    QCOMPARE(spy.count(), 0);
}

void tst_appcommclient::realtimeError_setsFailedAndEmitsError()
{
    QSignalSpy spy(m_app, &AppcommClient::errorOccurred);

    m_realtime->emitErrorOccurred("Socket failed");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("Socket failed"));
    QCOMPARE(m_app->connectionState(), ConnectionState::Failed);
    QCOMPARE(m_app->connectionStateText(), QString("Failed"));
}

void tst_appcommclient::recoveryMessagesRecovered_emitsMessageReceived()
{
    QJsonObject auth{{"$id", "session-1"}, {"userId", "user-1"}};
    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateAnonymousSession, auth);
    m_app->joinChannel("general");

    QSignalSpy spy(m_app, &AppcommClient::messageReceived);

    model::Message msg = makeIncomingMessage("general", "msg-rec-1", 1);
    QJsonArray arr;
    arr.append(msg.toJson());

    m_recovery->emitMessagesRecovered(arr);

    QCOMPARE(spy.count(), 1);
    const appcomm::model::Message received =
        qvariant_cast<appcomm::model::Message>(spy.at(0).at(0));
    QCOMPARE(received.messageId, QString("msg-rec-1"));
}

void tst_appcommclient::recoveryError_emitsError()
{
    QSignalSpy spy(m_app, &AppcommClient::errorOccurred);

    m_recovery->emitRecoveryError(500, "Recovery failed");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("Recovery failed"));
}

void tst_appcommclient::logout_authenticated_deletesSessionAndResetsState()
{
    QJsonObject data{{"$id", "session-logout"}, {"userId", "user-logout"}};
    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateAnonymousSession, data);

    QSignalSpy spy(m_app, &AppcommClient::authenticationStateChanged);

    m_app->logout();

    QVERIFY(!m_app->isAuthenticated());
    QCOMPARE(m_client->deleteSessionCalls, 1);
    QCOMPARE(m_client->lastDeletedSessionId, QString("session-logout"));
    QCOMPARE(spy.count(), 1);
}

void tst_appcommclient::logout_notAuthenticated_doesNothing()
{
    QSignalSpy spy(m_app, &AppcommClient::authenticationStateChanged);

    m_app->logout();

    QVERIFY(!m_app->isAuthenticated());
    QCOMPARE(m_client->deleteSessionCalls, 0);
    QCOMPARE(spy.count(), 0);
}

void tst_appcommclient::requestSuccess_nonAuthType_doesNothing()
{
    QSignalSpy spy(m_app, &AppcommClient::authenticationStateChanged);

    QJsonObject data{
        {"$id", "doc-1"},
        {"userId", "user-ignored"}
    };

    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateDocument, data);

    QVERIFY(!m_app->isAuthenticated());
    QCOMPARE(m_app->sessionInfo().sessionId, QString());
    QCOMPARE(m_app->sessionInfo().userId, QString());
    QCOMPARE(spy.count(), 0);
}

void tst_appcommclient::authSuccess_whenAlreadyAuthenticated_doesNotEmitAgain()
{
    QSignalSpy spy(m_app, &AppcommClient::authenticationStateChanged);

    QJsonObject firstData{
        {"$id", "session-1"},
        {"userId", "user-1"}
    };

    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateAnonymousSession, firstData);

    QCOMPARE(spy.count(), 1);
    QVERIFY(m_app->isAuthenticated());
    QCOMPARE(m_app->sessionInfo().sessionId, QString("session-1"));
    QCOMPARE(m_app->sessionInfo().userId, QString("user-1"));

    QJsonObject secondData{
        {"$id", "session-2"},
        {"userId", "user-2"}
    };

    m_client->emitRequestSuccess(appwritesdk::RequestType::CreateEmailSession, secondData);

    //Non deve riemettere il signal e non deve sovrascrivere i dati già presenti
    QCOMPARE(spy.count(), 1);
    QVERIFY(m_app->isAuthenticated());
    QCOMPARE(m_app->sessionInfo().sessionId, QString("session-1"));
    QCOMPARE(m_app->sessionInfo().userId, QString("user-1"));
}

QTEST_MAIN(tst_appcommclient)
#include "tst_appcommclient.moc"