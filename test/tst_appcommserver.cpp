#include <QTest>
#include <QSignalSpy>
#include "appcommserver.h"
#include "model.h"

using namespace appcomm::server;
using namespace appcomm::model;

class tst_appcommserver : public QObject
{
    Q_OBJECT

public:
    tst_appcommserver();
    ~tst_appcommserver();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Construction
    void testConstruction();
    void testDestructor();

    // Configuration
    void testConfigure();
    void testConfigureEmitsSignal();

    // Initialization state
    void testIsInitializedDefault();
    void testIsInitializedAfterInit();

    // Channel management
    void testCreateChannelValid();
    void testCreateChannelInvalid();
    void testCreateChannelEmitsSignal();
    void testDeleteChannelValid();
    void testDeleteChannelEmpty();
    void testDeleteChannelEmitsError();
    void testListChannelsEmpty();
    void testListChannelsWithData();
    void testGetChannelValid();
    void testGetChannelInvalid();
    void testChannelsGetter();

    // User management
    void testCreateUserValid();
    void testCreateUserEmptyEmail();
    void testCreateUserEmptyPassword();
    void testDeleteUserValid();
    void testDeleteUserEmpty();

    // Message management
    void testBroadcastMessageValid();
    void testBroadcastMessageInvalid();
    void testGetChannelMessagesValid();
    void testGetChannelMessagesEmpty();
    void testDeleteMessageValid();
    void testDeleteMessageEmpty();

    // Membership management
    void testAddChannelMemberValid();
    void testAddChannelMemberEmptyChannel();
    void testAddChannelMemberEmptyUser();
    void testRemoveChannelMemberValid();
    void testRemoveChannelMemberEmptyChannel();
    void testRemoveChannelMemberEmptyUser();
    void testGetChannelMembersValid();
    void testGetChannelMembersEmpty();

private:
    AppcommServer *server;
};

tst_appcommserver::tst_appcommserver() {}
tst_appcommserver::~tst_appcommserver() {}

void tst_appcommserver::initTestCase() {}
void tst_appcommserver::cleanupTestCase() {}

void tst_appcommserver::init()
{
    server = new AppcommServer();
}

void tst_appcommserver::cleanup()
{
    delete server;
    server = nullptr;
}

// Construction

void tst_appcommserver::testConstruction()
{
    AppcommServer *srv = new AppcommServer();
    QVERIFY(srv != nullptr);
    QVERIFY(!srv->isInitialized());
    delete srv;
}

void tst_appcommserver::testDestructor()
{
    AppcommServer *srv = new AppcommServer();
    delete srv;
    QVERIFY(true);
}

// Configuration

void tst_appcommserver::testConfigure()
{
    AppCommConfig config;
    config.endpoint = "https://example.com";
    config.projectId = "test-project";
    config.apiKey = "test-api-key";
    config.databaseId = "test-db";
    config.messagesCollectionId = "messages";
    config.membersCollectionId = "members";

    server->configure(config);
    QVERIFY(true);
}

void tst_appcommserver::testConfigureEmitsSignal()
{
    QSignalSpy spy(server, &AppcommServer::configured);

    AppCommConfig config;
    config.endpoint = "https://example.com";
    config.projectId = "test-project";
    config.apiKey = "test-api-key";
    config.databaseId = "test-db";
    config.messagesCollectionId = "messages";
    config.membersCollectionId = "members";

    server->configure(config);
    QCOMPARE(spy.count(), 1);
}

// Initialization state

void tst_appcommserver::testIsInitializedDefault()
{
    QVERIFY(!server->isInitialized());
}

void tst_appcommserver::testIsInitializedAfterInit()
{
    server->initialize();
    QVERIFY(!server->isInitialized());
}

// Channel management

void tst_appcommserver::testCreateChannelValid()
{
    Channel channel;
    channel.channelId = "test-channel-123";
    channel.name = "Test Channel";

    QSignalSpy spy(server, &AppcommServer::channelCreated);
    server->createChannel(channel);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    Channel emittedChannel = args.at(0).value<Channel>();
    QCOMPARE(emittedChannel.channelId, channel.channelId);
    QCOMPARE(emittedChannel.name, channel.name);
}

void tst_appcommserver::testCreateChannelInvalid()
{
    Channel channel;
    channel.channelId = "";
    channel.name = "Invalid Channel";

    QSignalSpy errorSpy(server, &AppcommServer::channelError);
    QSignalSpy createdSpy(server, &AppcommServer::channelCreated);

    server->createChannel(channel);

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(createdSpy.count(), 0);
}

void tst_appcommserver::testCreateChannelEmitsSignal()
{
    Channel channel;
    channel.channelId = "test-channel-456";
    channel.name = "Signal Test Channel";

    QSignalSpy spy(server, &AppcommServer::channelCreated);
    server->createChannel(channel);

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testDeleteChannelValid()
{
    Channel channel;
    channel.channelId = "channel-to-delete";
    channel.name = "Delete Me";
    server->createChannel(channel);

    server->deleteChannel("channel-to-delete");
    QVERIFY(true);
}

void tst_appcommserver::testDeleteChannelEmpty()
{
    QSignalSpy spy(server, &AppcommServer::channelError);
    server->deleteChannel("");

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toInt(), 400);
}

void tst_appcommserver::testDeleteChannelEmitsError()
{
    QSignalSpy spy(server, &AppcommServer::channelError);
    server->deleteChannel("");

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testListChannelsEmpty()
{
    QSignalSpy spy(server, &AppcommServer::channelsListed);
    server->listChannels();

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QList<Channel> channels = args.at(0).value<QList<Channel>>();
    QCOMPARE(channels.size(), 0);
}

void tst_appcommserver::testListChannelsWithData()
{
    Channel channel1;
    channel1.channelId = "channel-1";
    channel1.name = "Channel 1";

    Channel channel2;
    channel2.channelId = "channel-2";
    channel2.name = "Channel 2";

    server->createChannel(channel1);
    server->createChannel(channel2);

    QSignalSpy spy(server, &AppcommServer::channelsListed);
    server->listChannels();

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QList<Channel> channels = args.at(0).value<QList<Channel>>();
    QCOMPARE(channels.size(), 2);
}

void tst_appcommserver::testGetChannelValid()
{
    Channel channel;
    channel.channelId = "get-channel-test";
    channel.name = "Get Test";
    server->createChannel(channel);

    Channel retrieved = server->getChannel("get-channel-test");
    QCOMPARE(retrieved.channelId, channel.channelId);
    QCOMPARE(retrieved.name, channel.name);
}

void tst_appcommserver::testGetChannelInvalid()
{
    Channel retrieved = server->getChannel("non-existent-channel");
    QVERIFY(retrieved.channelId.isEmpty());
}

void tst_appcommserver::testChannelsGetter()
{
    Channel channel;
    channel.channelId = "getter-test";
    channel.name = "Getter Channel";
    server->createChannel(channel);

    QList<Channel> channels = server->channels();
    QCOMPARE(channels.size(), 1);
    QCOMPARE(channels.first().channelId, channel.channelId);
}

// User management

void tst_appcommserver::testCreateUserValid()
{
    server->createUser("test@example.com", "password123", "Test User");
    QVERIFY(true);
}

void tst_appcommserver::testCreateUserEmptyEmail()
{
    QSignalSpy spy(server, &AppcommServer::userError);
    server->createUser("", "password", "Test");

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testCreateUserEmptyPassword()
{
    QSignalSpy spy(server, &AppcommServer::userError);
    server->createUser("test@example.com", "", "Test");

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testDeleteUserValid()
{
    server->deleteUser("user-123");
    QVERIFY(true);
}

void tst_appcommserver::testDeleteUserEmpty()
{
    QSignalSpy spy(server, &AppcommServer::userError);
    server->deleteUser("");

    QCOMPARE(spy.count(), 1);
}

// Message management

void tst_appcommserver::testBroadcastMessageValid()
{
    Message msg;
    msg.channelId = "channel-1";
    msg.senderId = "user-1";
    msg.messageId = "msg-1";
    msg.sequenceNumber = 1;
    msg.timestamp = QDateTime::currentDateTimeUtc();
    msg.payload = QJsonObject{{"text", "Hello"}};

    server->broadcastMessage(msg);
    QVERIFY(true);
}

void tst_appcommserver::testBroadcastMessageInvalid()
{
    Message msg;
    msg.channelId = "";

    QSignalSpy spy(server, &AppcommServer::messageError);
    server->broadcastMessage(msg);

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testGetChannelMessagesValid()
{
    server->getChannelMessages("channel-1", 50);
    QVERIFY(true);
}

void tst_appcommserver::testGetChannelMessagesEmpty()
{
    QSignalSpy spy(server, &AppcommServer::messageError);
    server->getChannelMessages("", 50);

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testDeleteMessageValid()
{
    server->deleteMessage("msg-123");
    QVERIFY(true);
}

void tst_appcommserver::testDeleteMessageEmpty()
{
    QSignalSpy spy(server, &AppcommServer::messageError);
    server->deleteMessage("");

    QCOMPARE(spy.count(), 1);
}

// Membership management

void tst_appcommserver::testAddChannelMemberValid()
{
    QSignalSpy addedSpy(server, &AppcommServer::memberAdded);
    QSignalSpy errorSpy(server, &AppcommServer::membershipError);
    server->addChannelMember("channel-1", "user-1");

    QVERIFY(addedSpy.isValid());
    QCOMPARE(errorSpy.count(), 0);
}

void tst_appcommserver::testAddChannelMemberEmptyChannel()
{
    QSignalSpy spy(server, &AppcommServer::membershipError);
    server->addChannelMember("", "user-1");

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testAddChannelMemberEmptyUser()
{
    QSignalSpy spy(server, &AppcommServer::membershipError);
    server->addChannelMember("channel-1", "");

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testRemoveChannelMemberValid()
{
    server->removeChannelMember("channel-1", "user-1");
    QVERIFY(true);
}

void tst_appcommserver::testRemoveChannelMemberEmptyChannel()
{
    QSignalSpy spy(server, &AppcommServer::membershipError);
    server->removeChannelMember("", "user-1");

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testRemoveChannelMemberEmptyUser()
{
    QSignalSpy spy(server, &AppcommServer::membershipError);
    server->removeChannelMember("channel-1", "");

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testGetChannelMembersValid()
{
    server->getChannelMembers("channel-1");
    QVERIFY(true);
}

void tst_appcommserver::testGetChannelMembersEmpty()
{
    QSignalSpy spy(server, &AppcommServer::membershipError);
    server->getChannelMembers("");

    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(tst_appcommserver)
#include "tst_appcommserver.moc"
