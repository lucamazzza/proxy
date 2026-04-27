#include <QtTest>
#include <QSignalSpy>

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

#include "appcommclient.h"
#include "appwritesdk.h"
#include "model.h"
#include "ratelimiter.h"
#include "realtime.h"
#include "recoverymanager.h"
#include "state.h"

using namespace appcomm;
using namespace appcomm::client;

Q_DECLARE_METATYPE(appcomm::model::Message)

class FakeClientSdk : public QObject, public appwritesdk::IClientSdk
{
    Q_OBJECT

public:
    int createAnonymousSessionCalls = 0;
    int createEmailSessionCalls = 0;
    int deleteSessionCalls = 0;
    int deleteSessionsCalls = 0;
    int getAccountCalls = 0;
    int createDocumentCalls = 0;
    int listDocumentsCalls = 0;
    int getDocumentCalls = 0;
    int updateDocumentCalls = 0;
    int deleteDocumentCalls = 0;

    appwritesdk::ConnectionConfig lastConfig;
    QString lastEmail;
    QString lastPassword;
    QString lastSessionId;
    QString lastDocumentId;
    QJsonObject lastData;
    QJsonArray lastQueries;

    void createAnonymousSession(const appwritesdk::ConnectionConfig &config) override
    {
        createAnonymousSessionCalls++;
        lastConfig = config;
    }

    void createEmailSession(const appwritesdk::ConnectionConfig &config,
                            const QString &email,
                            const QString &password) override
    {
        createEmailSessionCalls++;
        lastConfig = config;
        lastEmail = email;
        lastPassword = password;
    }

    void deleteSession(const appwritesdk::ConnectionConfig &config,
                       const QString &sessionId) override
    {
        deleteSessionCalls++;
        lastConfig = config;
        lastSessionId = sessionId;
    }

    void deleteSessions(const appwritesdk::ConnectionConfig &config) override
    {
        deleteSessionsCalls++;
        lastConfig = config;
    }

    void getAccount(const appwritesdk::ConnectionConfig &config) override
    {
        getAccountCalls++;
        lastConfig = config;
    }

    void createDocument(const appwritesdk::ConnectionConfig &config,
                        const QJsonObject &data) override
    {
        createDocumentCalls++;
        lastConfig = config;
        lastData = data;
    }

    void listDocuments(const appwritesdk::ConnectionConfig &config,
                       const QJsonArray &queries = QJsonArray()) override
    {
        listDocumentsCalls++;
        lastConfig = config;
        lastQueries = queries;
    }

    void getDocument(const appwritesdk::ConnectionConfig &config,
                     const QString &documentId) override
    {
        getDocumentCalls++;
        lastConfig = config;
        lastDocumentId = documentId;
    }

    void updateDocument(const appwritesdk::ConnectionConfig &config,
                        const QString &documentId,
                        const QJsonObject &data) override
    {
        updateDocumentCalls++;
        lastConfig = config;
        lastDocumentId = documentId;
        lastData = data;
    }

    void deleteDocument(const appwritesdk::ConnectionConfig &config,
                        const QString &documentId) override
    {
        deleteDocumentCalls++;
        lastConfig = config;
        lastDocumentId = documentId;
    }

signals:
    void requestSuccess(const QJsonObject &data);
    void requestError(int code, const QString &message);
};

class FakeRealtime : public IRealtime
{
    Q_OBJECT

public:
    int connectToChannelsCalls = 0;
    int disconnectFromServerCalls = 0;
    QStringList lastChannels;

    explicit FakeRealtime(QObject *parent = nullptr)
        : IRealtime(parent)
    {
    }

    void connectToChannels(const QStringList &channels) override
    {
        connectToChannelsCalls++;
        lastChannels = channels;
    }

    void disconnectFromServer() override
    {
        disconnectFromServerCalls++;
    }
};

class FakeRateLimiter : public IRateLimiter
{
public:
    bool allowed = true;
    int allowRequestCalls = 0;
    int resetCalls = 0;

    bool allowRequest() override
    {
        allowRequestCalls++;
        return allowed;
    }

    void reset() override
    {
        resetCalls++;
    }
};

class FakeRecoveryManager : public QObject, public IRecoveryManager
{
    Q_OBJECT

public:
    int requestFromCalls = 0;
    QString lastMessageId;

    explicit FakeRecoveryManager(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    void requestFrom(const QString &messageId) override
    {
        requestFromCalls++;
        lastMessageId = messageId;
    }

signals:
    void messagesRecovered(const QJsonArray &messages);
    void recoveryError(int errorCode, const QString &errorMessage);
    void resyncCompleted(int messageCount);
};

class TestAppcommClient : public QObject
{
    Q_OBJECT

private:
    model::AppCommConfig validConfig() const
    {
        model::AppCommConfig cfg;
        cfg.endpoint = "http://localhost/v1";
        cfg.projectId = "project-id";
        cfg.databaseId = "database-id";
        cfg.messagesCollectionId = "messages";
        cfg.incomingMessagesCollectionId = "pendingmessages";
        cfg.membersCollectionId = "members";
        return cfg;
    }

    model::AppCommConfig configWithoutMessagesCollection() const
    {
        model::AppCommConfig cfg = validConfig();
        cfg.messagesCollectionId.clear();
        return cfg;
    }

    model::AppCommConfig configWithoutIncomingMessagesCollection() const
    {
        model::AppCommConfig cfg = validConfig();
        cfg.incomingMessagesCollectionId.clear();
        return cfg;
    }

    model::AppCommConfig configWithoutMembersCollection() const
    {
        model::AppCommConfig cfg = validConfig();
        cfg.membersCollectionId.clear();
        return cfg;
    }

    QJsonObject sessionResponse(const QString &sessionId = "session-1",
                                const QString &userId = "user-1",
                                const QString &provider = "anonymous") const
    {
        return {
            {"$id", sessionId},
            {"userId", userId},
            {"provider", provider},
            {"$createdAt", "2026-04-23T10:00:00.000Z"},
            {"expire", "2026-04-24T10:00:00.000Z"}
        };
    }

    QJsonObject membershipDocument(const QString &userId = "user-1",
                                   const QString &channelId = "channel-1") const
    {
        return {
            {"userId", userId},
            {"channelId", channelId},
            {"displayName", "Test User"},
            {"joinedAt", "2026-04-23T10:00:00.000Z"},
            {"lastSeenAt", "2026-04-23T10:00:00.000Z"},
            {"isActive", true}
        };
    }

    QJsonObject membershipResponse(const QJsonObject &member) const
    {
        return {
            {"documents", QJsonArray{member}}
        };
    }

    QJsonObject emptyDocumentsResponse() const
    {
        return {
            {"documents", QJsonArray{}}
        };
    }

    QJsonObject messageDocument(const QString &channelId = "channel-1",
                                qint64 sequenceNumber = 1,
                                const QString &messageId = "message-1") const
    {
        return {
            {"channelId", channelId},
            {"senderId", "sender-1"},
            {"messageId", messageId},
            {"sequenceNumber", sequenceNumber},
            {"timestamp", "2026-04-23T10:00:00.000Z"},
            {"payload", QJsonObject{{"text", "hello"}}},
            {"isEcho", false}
        };
    }

    QJsonObject messagesResponse(const QJsonArray &messages) const
    {
        return {
            {"documents", messages}
        };
    }

private slots:
    void initTestCase()
    {
        qRegisterMetaType<appcomm::model::Message>("appcomm::model::Message");
    }

    void initialState_isCorrect()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);

        QCOMPARE(client.connectionState(), ConnectionState::Disconnected);
        QCOMPARE(client.connectionStateText(), QString("Disconnected"));
        QCOMPARE(client.isAuthenticated(), false);
        QCOMPARE(client.activeChannel(), QString());
        QVERIFY(client.sessionInfo().sessionId.isEmpty());
        QVERIFY(client.sessionInfo().userId.isEmpty());
    }

    void createGuestSession_callsSdk()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);

        client.createGuestSession();

        QCOMPARE(sdk.createAnonymousSessionCalls, 1);
        QCOMPARE(sdk.lastConfig.endpoint, QString("http://localhost/v1"));
        QCOMPARE(sdk.lastConfig.projectId, QString("project-id"));
        QCOMPARE(sdk.lastConfig.dbId, QString("database-id"));
        QCOMPARE(sdk.lastConfig.collectionId, QString("messages"));
    }

    void createEmailSession_callsSdkWithCredentials()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);

        client.createEmailSession("test@example.com", "secret");

        QCOMPARE(sdk.createEmailSessionCalls, 1);
        QCOMPARE(sdk.lastEmail, QString("test@example.com"));
        QCOMPARE(sdk.lastPassword, QString("secret"));
    }

    void loginSuccess_setsAuthenticatedSessionAndLoadsMembership()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy authSpy(&client, &AppcommClient::authenticationStateChanged);

        client.createGuestSession();
        emit sdk.requestSuccess(sessionResponse("session-1", "user-1", "anonymous"));

        QCOMPARE(client.isAuthenticated(), true);
        QCOMPARE(client.sessionInfo().sessionId, QString("session-1"));
        QCOMPARE(client.sessionInfo().userId, QString("user-1"));
        QCOMPARE(client.sessionInfo().authType, model::AuthType::Guest);
        QCOMPARE(authSpy.count(), 1);

        QCOMPARE(sdk.listDocumentsCalls, 1);
        QCOMPARE(sdk.lastConfig.collectionId, QString("members"));
        QCOMPARE(sdk.lastQueries.size(), 2);
        QCOMPARE(sdk.lastQueries.at(0).toString(), QString("equal(\"userId\",[\"user-1\"])"));
        QCOMPARE(sdk.lastQueries.at(1).toString(), QString("limit(1)"));
    }

    void emailLoginSuccess_setsEmailAuthType()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);

        client.createEmailSession("test@example.com", "secret");
        emit sdk.requestSuccess(sessionResponse("session-1", "user-1", "email"));

        QCOMPARE(client.isAuthenticated(), true);
        QCOMPARE(client.sessionInfo().authType, model::AuthType::Email);
    }

    void invalidLoginResponse_doesNothing()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy authSpy(&client, &AppcommClient::authenticationStateChanged);

        client.createGuestSession();
        emit sdk.requestSuccess(QJsonObject{
            {"$id", ""},
            {"userId", "user-1"}
        });

        QCOMPARE(client.isAuthenticated(), false);
        QCOMPARE(authSpy.count(), 0);
        QCOMPARE(sdk.listDocumentsCalls, 0);
    }

    void requestError_resetsPendingRequestAndEmitsError()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy errorSpy(&client, &AppcommClient::errorOccurred);

        client.createGuestSession();
        emit sdk.requestError(401, "Unauthorized");

        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(), QString("Unauthorized"));

        emit sdk.requestSuccess(sessionResponse());
        QCOMPARE(client.isAuthenticated(), false);
    }

    void loginSuccess_withoutMembersCollection_emitsError()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(configWithoutMembersCollection(),
                             &sdk, &realtime, &recovery, &limiter);

        QSignalSpy errorSpy(&client, &AppcommClient::errorOccurred);

        client.createGuestSession();
        emit sdk.requestSuccess(sessionResponse());

        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(),
                 QString("Cannot load membership: members collection is not configured."));
    }

    void membershipSuccess_setsChannelAndLoadsMessages()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy joinedSpy(&client, &AppcommClient::joinedChannel);

        client.createGuestSession();
        emit sdk.requestSuccess(sessionResponse("session-1", "user-1"));

        emit sdk.requestSuccess(
            membershipResponse(membershipDocument("user-1", "channel-42"))
            );

        QCOMPARE(client.activeChannel(), QString("channel-42"));
        QCOMPARE(joinedSpy.count(), 1);
        QCOMPARE(joinedSpy.first().at(0).toString(), QString("channel-42"));

        QCOMPARE(sdk.listDocumentsCalls, 2);
        QCOMPARE(sdk.lastConfig.collectionId, QString("messages"));
        QCOMPARE(sdk.lastQueries.size(), 3);
        QCOMPARE(sdk.lastQueries.at(0).toString(),
                 QString("equal(\"channelId\",[\"channel-42\"])"));
        QCOMPARE(sdk.lastQueries.at(1).toString(),
                 QString("orderAsc(\"sequenceNumber\")"));
        QCOMPARE(sdk.lastQueries.at(2).toString(),
                 QString("limit(50)"));
    }

    void membershipEmpty_emitsError()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy errorSpy(&client, &AppcommClient::errorOccurred);

        client.createGuestSession();
        emit sdk.requestSuccess(sessionResponse());
        emit sdk.requestSuccess(emptyDocumentsResponse());

        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(),
                 QString("No membership found for current user."));
    }

    void invalidMembership_emitsError()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy errorSpy(&client, &AppcommClient::errorOccurred);

        client.createGuestSession();
        emit sdk.requestSuccess(sessionResponse());

        emit sdk.requestSuccess(
            membershipResponse(QJsonObject{
                {"userId", "user-1"},
                {"channelId", ""}
            })
            );

        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(),
                 QString("Invalid membership document."));
    }

    void joinChannel_trimsChannelEmitsSignalAndLoadsMessages()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy joinedSpy(&client, &AppcommClient::joinedChannel);

        client.joinChannel("  channel-1  ");

        QCOMPARE(client.activeChannel(), QString("channel-1"));
        QCOMPARE(joinedSpy.count(), 1);
        QCOMPARE(joinedSpy.first().at(0).toString(), QString("channel-1"));

        QCOMPARE(sdk.listDocumentsCalls, 1);
        QCOMPARE(sdk.lastConfig.collectionId, QString("messages"));
    }

    void joinChannel_emptyEmitsError()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy errorSpy(&client, &AppcommClient::errorOccurred);

        client.joinChannel("   ");

        QCOMPARE(client.activeChannel(), QString());
        QCOMPARE(sdk.listDocumentsCalls, 0);
        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(), QString("Channel ID cannot be empty."));
    }

    void leaveChannel_withoutChannelDoesNothing()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy leftSpy(&client, &AppcommClient::leftChannel);

        client.leaveChannel();

        QCOMPARE(leftSpy.count(), 0);
    }

    void leaveChannel_withChannelClearsAndEmitsSignal()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy leftSpy(&client, &AppcommClient::leftChannel);

        client.joinChannel("channel-1");
        client.leaveChannel();

        QCOMPARE(client.activeChannel(), QString());
        QCOMPARE(leftSpy.count(), 1);
        QCOMPARE(leftSpy.first().at(0).toString(), QString("channel-1"));
    }

    void connectToServer_withoutChannelDoesNothing()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);

        client.connectToServer();

        QCOMPARE(client.connectionState(), ConnectionState::Disconnected);
        QCOMPARE(realtime.connectToChannelsCalls, 0);
    }

    void connectToServer_withChannelConnectsRealtime()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy stateSpy(&client, &AppcommClient::connectionStateChanged);

        client.joinChannel("channel-1");
        client.connectToServer();

        QCOMPARE(client.connectionState(), ConnectionState::Connecting);
        QCOMPARE(stateSpy.count(), 1);
        QCOMPARE(realtime.connectToChannelsCalls, 1);
        QCOMPARE(realtime.lastChannels.first(),
                 QString("databases.database-id.collections.messages.documents"));
    }

    void connectToServer_whenAlreadyConnectingDoesNothing()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);

        client.joinChannel("channel-1");
        client.connectToServer();
        client.connectToServer();

        QCOMPARE(realtime.connectToChannelsCalls, 1);
    }

    void connectToServer_withoutMessagesCollectionEmitsError()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(configWithoutMessagesCollection(),
                             &sdk, &realtime, &recovery, &limiter);

        QSignalSpy errorSpy(&client, &AppcommClient::errorOccurred);

        client.joinChannel("channel-1");
        client.connectToServer();

        QCOMPARE(errorSpy.last().at(0).toString(),
                 QString("Cannot connect to server: messages collection is not configured."));
    }

    void realtimeConnected_setsConnected()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);

        emit realtime.connected();

        QCOMPARE(client.connectionState(), ConnectionState::Connected);
        QCOMPARE(client.connectionStateText(), QString("Connected"));
    }

    void realtimeDisconnected_setsDisconnected()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);

        emit realtime.connected();
        emit realtime.disconnected();

        QCOMPARE(client.connectionState(), ConnectionState::Disconnected);
        QCOMPARE(client.connectionStateText(), QString("Disconnected"));
    }

    void disconnectFromServer_whenDisconnectedDoesNothing()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);

        client.disconnectFromServer();

        QCOMPARE(realtime.disconnectFromServerCalls, 0);
        QCOMPARE(client.connectionState(), ConnectionState::Disconnected);
    }

    void disconnectFromServer_whenConnectedDisconnectsRealtime()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);

        emit realtime.connected();

        client.disconnectFromServer();

        QCOMPARE(client.connectionState(), ConnectionState::Disconnecting);
        QCOMPARE(client.connectionStateText(), QString("Disconnecting"));
        QCOMPARE(realtime.disconnectFromServerCalls, 1);
    }

    void leaveChannel_whenConnectedDisconnects()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);

        client.joinChannel("channel-1");
        emit realtime.connected();

        client.leaveChannel();

        QCOMPARE(client.activeChannel(), QString());
        QCOMPARE(client.connectionState(), ConnectionState::Disconnecting);
        QCOMPARE(realtime.disconnectFromServerCalls, 1);
    }

    void sendMessage_whenNotAuthenticatedEmitsError()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy errorSpy(&client, &AppcommClient::errorOccurred);

        client.joinChannel("channel-1");
        client.sendMessage(QJsonObject{{"text", "hello"}});

        QCOMPARE(errorSpy.last().at(0).toString(),
                 QString("Cannot send message: client is not authenticated."));
        QCOMPARE(sdk.createDocumentCalls, 0);
    }

    void sendMessage_withoutChannelEmitsError()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy errorSpy(&client, &AppcommClient::errorOccurred);

        client.createGuestSession();
        emit sdk.requestSuccess(sessionResponse());

        client.sendMessage(QJsonObject{{"text", "hello"}});

        QCOMPARE(errorSpy.last().at(0).toString(),
                 QString("Cannot send message: no active channel."));
        QCOMPARE(sdk.createDocumentCalls, 0);
    }

    void sendMessage_whenRateLimitedEmitsError()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;
        limiter.allowed = false;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy errorSpy(&client, &AppcommClient::errorOccurred);

        client.createGuestSession();
        emit sdk.requestSuccess(sessionResponse());

        client.joinChannel("channel-1");
        client.sendMessage(QJsonObject{{"text", "hello"}});

        QCOMPARE(limiter.allowRequestCalls, 1);
        QCOMPARE(errorSpy.last().at(0).toString(), QString("Rate limit exceeded."));
        QCOMPARE(sdk.createDocumentCalls, 0);
    }

    void sendMessage_withoutIncomingCollectionEmitsError()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(configWithoutIncomingMessagesCollection(),
                             &sdk, &realtime, &recovery, &limiter);

        QSignalSpy errorSpy(&client, &AppcommClient::errorOccurred);

        client.createGuestSession();
        emit sdk.requestSuccess(sessionResponse());

        client.joinChannel("channel-1");
        client.sendMessage(QJsonObject{{"text", "hello"}});

        QCOMPARE(errorSpy.last().at(0).toString(),
                 QString("Cannot send message: incoming messages collection is not configured."));
        QCOMPARE(sdk.createDocumentCalls, 0);
    }

    void sendMessage_whenValidCreatesPendingMessage()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);

        client.createGuestSession();
        emit sdk.requestSuccess(sessionResponse("session-1", "user-1"));

        client.joinChannel("channel-1");
        client.sendMessage(QJsonObject{{"text", "hello"}});

        QCOMPARE(limiter.allowRequestCalls, 1);
        QCOMPARE(sdk.createDocumentCalls, 1);
        QCOMPARE(sdk.lastConfig.collectionId, QString("incomingMessages"));

        QCOMPARE(sdk.lastData.value("channelId").toString(), QString("channel-1"));
        QCOMPARE(sdk.lastData.value("senderId").toString(), QString("user-1"));
        QVERIFY(!sdk.lastData.value("messageId").toString().isEmpty());
        QVERIFY(sdk.lastData.value("payload").isString());

        const QString payloadStr = sdk.lastData.value("payload").toString();

        const QJsonDocument payloadDoc = QJsonDocument::fromJson(payloadStr.toUtf8());
        QVERIFY(payloadDoc.isObject());

        const QJsonObject payloadObj = payloadDoc.object();

        QCOMPARE(payloadObj.value("text").toString(), QString("hello"));
    }

    void loadMembership_withoutUserIdEmitsError()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy errorSpy(&client, &AppcommClient::errorOccurred);

        client.loadMembership();

        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(),
                 QString("Cannot load membership: missing user ID."));
    }

    void loadChannelMessages_withoutChannelEmitsError()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy errorSpy(&client, &AppcommClient::errorOccurred);

        client.loadChannelMessages();

        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(),
                 QString("Cannot load messages: no active channel."));
    }

    void loadChannelMessages_customLimitIsUsed()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);

        client.joinChannel("channel-1");
        client.loadChannelMessages(12);

        QCOMPARE(sdk.lastQueries.at(2).toString(), QString("limit(12)"));
    }

    void loadChannelMessages_invalidLimitFallsBackTo50()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);

        client.joinChannel("channel-1");
        client.loadChannelMessages(0);

        QCOMPARE(sdk.lastQueries.at(2).toString(), QString("limit(50)"));
    }

    void messagesLoaded_emitsMessagesAndConnects()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy messageSpy(&client, &AppcommClient::messageReceived);

        client.joinChannel("channel-1");

        emit sdk.requestSuccess(messagesResponse(QJsonArray{
            messageDocument("channel-1", 1, "msg-1"),
            messageDocument("channel-1", 2, "msg-2")
        }));

        QCOMPARE(messageSpy.count(), 2);
        QCOMPARE(realtime.connectToChannelsCalls, 1);
        QCOMPARE(client.connectionState(), ConnectionState::Connecting);

        const auto first =
            qvariant_cast<appcomm::model::Message>(messageSpy.at(0).at(0));
        const auto second =
            qvariant_cast<appcomm::model::Message>(messageSpy.at(1).at(0));

        QCOMPARE(first.messageId, QString("msg-1"));
        QCOMPARE(first.sequenceNumber, 1);
        QCOMPARE(second.messageId, QString("msg-2"));
        QCOMPARE(second.sequenceNumber, 2);
    }

    void messagesLoaded_ignoresInvalidWrongChannelAndDuplicates()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy messageSpy(&client, &AppcommClient::messageReceived);

        client.joinChannel("channel-1");

        QJsonObject invalid = messageDocument("channel-1", 1, "invalid");
        invalid.remove("messageId");

        emit sdk.requestSuccess(messagesResponse(QJsonArray{
            invalid,
            messageDocument("other-channel", 1, "wrong-channel"),
            messageDocument("channel-1", 1, "msg-1"),
            messageDocument("channel-1", 1, "duplicate")
        }));

        QCOMPARE(messageSpy.count(), 1);

        const auto message =
            qvariant_cast<appcomm::model::Message>(messageSpy.first().at(0));

        QCOMPARE(message.messageId, QString("msg-1"));
    }

    void realtimeEvent_directPayloadEmitsMessage()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy messageSpy(&client, &AppcommClient::messageReceived);

        client.joinChannel("channel-1");

        emit realtime.eventReceived(QJsonObject{
            {"payload", messageDocument("channel-1", 1, "rt-1")}
        });

        QCOMPARE(messageSpy.count(), 1);

        const auto message =
            qvariant_cast<appcomm::model::Message>(messageSpy.first().at(0));

        QCOMPARE(message.messageId, QString("rt-1"));
    }

    void realtimeEvent_nestedPayloadEmitsMessage()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy messageSpy(&client, &AppcommClient::messageReceived);

        client.joinChannel("channel-1");

        emit realtime.eventReceived(QJsonObject{
            {"data", QJsonObject{
                         {"payload", QJsonObject{
                                         {"data", messageDocument("channel-1", 1, "nested-1")}
                                     }}
                     }}
        });

        QCOMPARE(messageSpy.count(), 1);

        const auto message =
            qvariant_cast<appcomm::model::Message>(messageSpy.first().at(0));

        QCOMPARE(message.messageId, QString("nested-1"));
    }

    void realtimeEvent_withoutPayloadIsIgnored()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy messageSpy(&client, &AppcommClient::messageReceived);

        client.joinChannel("channel-1");

        emit realtime.eventReceived(QJsonObject{
            {"events", QJsonArray{"something"}}
        });

        QCOMPARE(messageSpy.count(), 0);
    }

    void realtimeError_setsFailedAndEmitsError()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy errorSpy(&client, &AppcommClient::errorOccurred);

        emit realtime.errorOccurred("WebSocket failed");

        QCOMPARE(client.connectionState(), ConnectionState::Failed);
        QCOMPARE(client.connectionStateText(), QString("Failed"));
        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(), QString("WebSocket failed"));
    }

    void recoveredMessages_emitMessageReceived()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy messageSpy(&client, &AppcommClient::messageReceived);

        client.joinChannel("channel-1");

        emit recovery.messagesRecovered(QJsonArray{
            messageDocument("channel-1", 1, "recovered-1"),
            messageDocument("channel-1", 2, "recovered-2")
        });

        QCOMPARE(messageSpy.count(), 2);
    }

    void recoveredMessages_ignoreInvalidValues()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy messageSpy(&client, &AppcommClient::messageReceived);

        client.joinChannel("channel-1");

        emit recovery.messagesRecovered(QJsonArray{
            QString("not-an-object"),
            messageDocument("channel-1", 1, "recovered-1")
        });

        QCOMPARE(messageSpy.count(), 1);
    }

    void recoveryError_emitsError()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy errorSpy(&client, &AppcommClient::errorOccurred);

        emit recovery.recoveryError(500, "Recovery failed");

        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(), QString("Recovery failed"));
    }

    void logout_whenAuthenticatedDeletesSessionAndClearsState()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);
        QSignalSpy authSpy(&client, &AppcommClient::authenticationStateChanged);

        client.createGuestSession();
        emit sdk.requestSuccess(sessionResponse("session-1", "user-1"));

        client.joinChannel("channel-1");

        QCOMPARE(client.isAuthenticated(), true);
        QCOMPARE(client.activeChannel(), QString("channel-1"));

        client.logout();

        QCOMPARE(sdk.deleteSessionCalls, 1);
        QCOMPARE(sdk.lastSessionId, QString("session-1"));

        QCOMPARE(client.isAuthenticated(), false);
        QCOMPARE(client.activeChannel(), QString());
        QVERIFY(client.sessionInfo().sessionId.isEmpty());
        QVERIFY(client.sessionInfo().userId.isEmpty());
        QVERIFY(authSpy.count() >= 2);
    }

    void logout_whenConnectedDisconnectsRealtime()
    {
        FakeClientSdk sdk;
        FakeRealtime realtime;
        FakeRecoveryManager recovery;
        FakeRateLimiter limiter;

        AppcommClient client(validConfig(), &sdk, &realtime, &recovery, &limiter);

        client.createGuestSession();
        emit sdk.requestSuccess(sessionResponse("session-1", "user-1"));

        client.joinChannel("channel-1");
        emit realtime.connected();

        client.logout();

        QCOMPARE(client.connectionState(), ConnectionState::Disconnecting);
        QCOMPARE(realtime.disconnectFromServerCalls, 1);
        QCOMPARE(client.isAuthenticated(), false);
        QCOMPARE(client.activeChannel(), QString());
    }
};

QTEST_MAIN(TestAppcommClient)

#include "tst_appcommclient.moc"