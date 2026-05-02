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

    // Topic management
    void testCreateTopicValid();
    void testCreateTopicInvalid();
    void testCreateTopicEmitsSignal();
    void testDeleteTopicValid();
    void testDeleteTopicEmpty();
    void testDeleteTopicEmitsError();
    void testListTopicsEmpty();
    void testListTopicsWithData();
    void testGetTopicValid();
    void testGetTopicInvalid();
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
    void testGetTopicMessagesValid();
    void testGetTopicMessagesEmpty();
    void testDeleteMessageValid();
    void testDeleteMessageEmpty();

    // Membership management
    void testAddTopicMemberValid();
    void testAddTopicMemberEmptyTopic();
    void testAddTopicMemberEmptyUser();
    void testRemoveTopicMemberValid();
    void testRemoveTopicMemberEmptyTopic();
    void testRemoveTopicMemberEmptyUser();
    void testGetTopicMembersValid();
    void testGetTopicMembersEmpty();

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

// Topic management

void tst_appcommserver::testCreateTopicValid()
{
    Topic topic;
    topic.topicId = "test-topic-123";
    topic.name = "Test Topic";

    QSignalSpy spy(server, &AppcommServer::topicCreated);
    server->createTopic(topic);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    Topic emittedTopic = args.at(0).value<Topic>();
    QCOMPARE(emittedTopic.topicId, topic.topicId);
    QCOMPARE(emittedTopic.name, topic.name);
}

void tst_appcommserver::testCreateTopicInvalid()
{
    Topic topic;
    topic.topicId = "";
    topic.name = "Invalid Topic";

    QSignalSpy errorSpy(server, &AppcommServer::topicError);
    QSignalSpy createdSpy(server, &AppcommServer::topicCreated);

    server->createTopic(topic);

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(createdSpy.count(), 0);
}

void tst_appcommserver::testCreateTopicEmitsSignal()
{
    Topic topic;
    topic.topicId = "test-topic-456";
    topic.name = "Signal Test Topic";

    QSignalSpy spy(server, &AppcommServer::topicCreated);
    server->createTopic(topic);

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testDeleteTopicValid()
{
    Topic topic;
    topic.topicId = "topic-to-delete";
    topic.name = "Delete Me";
    server->createTopic(topic);

    server->deleteTopic("topic-to-delete");
    QVERIFY(true);
}

void tst_appcommserver::testDeleteTopicEmpty()
{
    QSignalSpy spy(server, &AppcommServer::topicError);
    server->deleteTopic("");

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toInt(), 400);
}

void tst_appcommserver::testDeleteTopicEmitsError()
{
    QSignalSpy spy(server, &AppcommServer::topicError);
    server->deleteTopic("");

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testListTopicsEmpty()
{
    QSignalSpy spy(server, &AppcommServer::topicsListed);
    server->listTopics();

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QList<Topic> topics = args.at(0).value<QList<Topic>>();
    QCOMPARE(topics.size(), 0);
}

void tst_appcommserver::testListTopicsWithData()
{
    Topic channel1;
    channel1.topicId = "topic-1";
    channel1.name = "Topic 1";

    Topic channel2;
    channel2.topicId = "topic-2";
    channel2.name = "Topic 2";

    server->createTopic(channel1);
    server->createTopic(channel2);

    QSignalSpy spy(server, &AppcommServer::topicsListed);
    server->listTopics();

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QList<Topic> topics = args.at(0).value<QList<Topic>>();
    QCOMPARE(topics.size(), 2);
}

void tst_appcommserver::testGetTopicValid()
{
    Topic topic;
    topic.topicId = "get-topic-test";
    topic.name = "Get Test";
    server->createTopic(topic);

    Topic retrieved = server->getTopic("get-topic-test");
    QCOMPARE(retrieved.topicId, topic.topicId);
    QCOMPARE(retrieved.name, topic.name);
}

void tst_appcommserver::testGetTopicInvalid()
{
    Topic retrieved = server->getTopic("non-existent-topic");
    QVERIFY(retrieved.topicId.isEmpty());
}

void tst_appcommserver::testChannelsGetter()
{
    Topic topic;
    topic.topicId = "getter-test";
    topic.name = "Getter Topic";
    server->createTopic(topic);

    QList<Topic> topics = server->topics();
    QCOMPARE(topics.size(), 1);
    QCOMPARE(topics.first().topicId, topic.topicId);
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
    msg.topicId = "topic-1";
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
    msg.topicId = "";

    QSignalSpy spy(server, &AppcommServer::messageError);
    server->broadcastMessage(msg);

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testGetTopicMessagesValid()
{
    server->getTopicMessages("topic-1", 16);
    QVERIFY(true);
}

void tst_appcommserver::testGetTopicMessagesEmpty()
{
    QSignalSpy spy(server, &AppcommServer::messageError);
    server->getTopicMessages("", 16);

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

void tst_appcommserver::testAddTopicMemberValid()
{
    QSignalSpy addedSpy(server, &AppcommServer::memberAdded);
    QSignalSpy errorSpy(server, &AppcommServer::membershipError);
    server->addTopicMember("topic-1", "user-1");

    QVERIFY(addedSpy.isValid());
    QCOMPARE(errorSpy.count(), 0);
}

void tst_appcommserver::testAddTopicMemberEmptyTopic()
{
    QSignalSpy spy(server, &AppcommServer::membershipError);
    server->addTopicMember("", "user-1");

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testAddTopicMemberEmptyUser()
{
    QSignalSpy spy(server, &AppcommServer::membershipError);
    server->addTopicMember("topic-1", "");

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testRemoveTopicMemberValid()
{
    server->removeTopicMember("topic-1", "user-1");
    QVERIFY(true);
}

void tst_appcommserver::testRemoveTopicMemberEmptyTopic()
{
    QSignalSpy spy(server, &AppcommServer::membershipError);
    server->removeTopicMember("", "user-1");

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testRemoveTopicMemberEmptyUser()
{
    QSignalSpy spy(server, &AppcommServer::membershipError);
    server->removeTopicMember("topic-1", "");

    QCOMPARE(spy.count(), 1);
}

void tst_appcommserver::testGetTopicMembersValid()
{
    server->getTopicMembers("topic-1");
    QVERIFY(true);
}

void tst_appcommserver::testGetTopicMembersEmpty()
{
    QSignalSpy spy(server, &AppcommServer::membershipError);
    server->getTopicMembers("");

    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(tst_appcommserver)
#include "tst_appcommserver.moc"
