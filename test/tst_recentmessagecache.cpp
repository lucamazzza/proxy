#include <QTest>
#include <QJsonObject>
#include <QDateTime>
#include "recentmessagecache.h"
#include "model.h"

using namespace appcomm;
using namespace appcomm::client;

class tst_recentmessagecache : public QObject
{
    Q_OBJECT

public:
    tst_recentmessagecache();
    ~tst_recentmessagecache();

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic functionality
    void initState();
    void addSingleMessage();
    void addMultipleMessages();
    void updateExistingMessage();
    void checkContains();
    void getMessage();
    void getAllMessages();

    // Capacity management
    void respectCapacity();
    void evictOldestMessage();
    void customCapacity();

    // Recovery functionality
    void getMessagesSinceValid();
    void getMessagesSinceInvalid();
    void getMessagesSinceEmpty();
    void getMessagesSinceLast();

    // Edge cases
    void addEmptyMessageId();
    void zeroCapacity();
    void clearCache();
    void duplicateMessages();

    // Ordering
    void maintainInsertionOrder();
    void orderAfterEviction();
};

tst_recentmessagecache::tst_recentmessagecache() {}
tst_recentmessagecache::~tst_recentmessagecache() {}
void tst_recentmessagecache::initTestCase() {}
void tst_recentmessagecache::cleanupTestCase() {}

// Basic functionality
void tst_recentmessagecache::initState() {
    RecentMessageCache cache(16);
    QCOMPARE(cache.size(), 0);
    QCOMPARE(cache.capacity(), 16);
    QVERIFY(cache.isEmpty());
}

void tst_recentmessagecache::addSingleMessage() {
    RecentMessageCache cache;
    model::Message msg;
    msg.messageId = "msg1";
    msg.channelId = "channel1";
    msg.senderId = "user1";
    msg.timestamp = QDateTime::currentDateTime();
    msg.payload = QJsonObject{{"text", "Hello"}};
    
    cache.addMessage(msg);
    
    QCOMPARE(cache.size(), 1);
    QVERIFY(!cache.isEmpty());
    QVERIFY(cache.contains("msg1"));
}

void tst_recentmessagecache::addMultipleMessages() {
    RecentMessageCache cache;
    
    for (int i = 0; i < 10; i++) {
        model::Message msg;
        msg.messageId = QString("msg%1").arg(i);
        msg.channelId = "channel1";
        msg.senderId = "user1";
        msg.timestamp = QDateTime::currentDateTime();
        msg.payload = QJsonObject{{"index", i}};
        cache.addMessage(msg);
    }
    
    QCOMPARE(cache.size(), 10);
    QVERIFY(cache.contains("msg0"));
    QVERIFY(cache.contains("msg9"));
}

void tst_recentmessagecache::updateExistingMessage() {
    RecentMessageCache cache;
    
    model::Message msg1;
    msg1.messageId = "msg1";
    msg1.channelId = "channel1";
    msg1.senderId = "user1";
    msg1.timestamp = QDateTime::currentDateTime();
    msg1.payload = QJsonObject{{"text", "Original"}};
    
    cache.addMessage(msg1);
    QCOMPARE(cache.size(), 1);
    
    model::Message msg2;
    msg2.messageId = "msg1";
    msg2.channelId = "channel1";
    msg2.senderId = "user1";
    msg2.timestamp = QDateTime::currentDateTime();
    msg2.payload = QJsonObject{{"text", "Updated"}};
    
    cache.addMessage(msg2);
    QCOMPARE(cache.size(), 1);
    
    model::Message msg = cache.getMessage("msg1");
    QCOMPARE(msg.payload.value("text").toString(), QString("Updated"));
}

void tst_recentmessagecache::checkContains() {
    RecentMessageCache cache;
    
    QVERIFY(!cache.contains("msg1"));
    
    model::Message msg;
    msg.messageId = "msg1";
    msg.channelId = "channel1";
    msg.senderId = "user1";
    msg.timestamp = QDateTime::currentDateTime();
    msg.payload = QJsonObject{{"test", true}};
    
    cache.addMessage(msg);
    QVERIFY(cache.contains("msg1"));
    QVERIFY(!cache.contains("msg2"));
}

void tst_recentmessagecache::getMessage() {
    RecentMessageCache cache;
    
    model::Message msgIn;
    msgIn.messageId = "msg1";
    msgIn.channelId = "channel1";
    msgIn.senderId = "user1";
    msgIn.timestamp = QDateTime::currentDateTime();
    msgIn.payload = QJsonObject{{"value", 42}};
    
    cache.addMessage(msgIn);
    
    model::Message msgOut = cache.getMessage("msg1");
    QCOMPARE(msgOut.messageId, QString("msg1"));
    QCOMPARE(msgOut.payload.value("value").toInt(), 42);
}

void tst_recentmessagecache::getAllMessages() {
    RecentMessageCache cache;
    
    for (int i = 0; i < 5; i++) {
        model::Message msg;
        msg.messageId = QString("msg%1").arg(i);
        msg.channelId = "channel1";
        msg.senderId = "user1";
        msg.timestamp = QDateTime::currentDateTime();
        msg.payload = QJsonObject{{"index", i}};
        cache.addMessage(msg);
    }
    
    QList<model::Message> messages = cache.getAllMessages();
    QCOMPARE(messages.size(), 5);
    QCOMPARE(messages[0].messageId, QString("msg0"));
    QCOMPARE(messages[4].messageId, QString("msg4"));
}

// Capacity management
void tst_recentmessagecache::respectCapacity() {
    RecentMessageCache cache(3);
    
    for (int i = 0; i < 3; i++) {
        model::Message msg;
        msg.messageId = QString("msg%1").arg(i);
        msg.channelId = "channel1";
        msg.senderId = "user1";
        msg.timestamp = QDateTime::currentDateTime();
        msg.payload = QJsonObject();
        cache.addMessage(msg);
    }
    
    QCOMPARE(cache.size(), 3);
}

void tst_recentmessagecache::evictOldestMessage() {
    RecentMessageCache cache(3);
    
    model::Message msg0;
    msg0.messageId = "msg0";
    msg0.channelId = "channel1";
    msg0.senderId = "user1";
    msg0.timestamp = QDateTime::currentDateTime();
    msg0.payload = QJsonObject{{"value", 0}};
    cache.addMessage(msg0);
    
    model::Message msg1;
    msg1.messageId = "msg1";
    msg1.channelId = "channel1";
    msg1.senderId = "user1";
    msg1.timestamp = QDateTime::currentDateTime();
    msg1.payload = QJsonObject{{"value", 1}};
    cache.addMessage(msg1);
    
    model::Message msg2;
    msg2.messageId = "msg2";
    msg2.channelId = "channel1";
    msg2.senderId = "user1";
    msg2.timestamp = QDateTime::currentDateTime();
    msg2.payload = QJsonObject{{"value", 2}};
    cache.addMessage(msg2);
    
    QVERIFY(cache.contains("msg0"));
    
    model::Message msg3;
    msg3.messageId = "msg3";
    msg3.channelId = "channel1";
    msg3.senderId = "user1";
    msg3.timestamp = QDateTime::currentDateTime();
    msg3.payload = QJsonObject{{"value", 3}};
    cache.addMessage(msg3);
    
    QCOMPARE(cache.size(), 3);
    QVERIFY(!cache.contains("msg0"));
    QVERIFY(cache.contains("msg1"));
    QVERIFY(cache.contains("msg2"));
    QVERIFY(cache.contains("msg3"));
}

void tst_recentmessagecache::customCapacity() {
    RecentMessageCache cache(200);
    QCOMPARE(cache.capacity(), 200);
    
    RecentMessageCache defaultCache;
    QCOMPARE(defaultCache.capacity(), 100);
}

// Recovery functionality
void tst_recentmessagecache::getMessagesSinceValid() {
    RecentMessageCache cache;
    
    for (int i = 0; i < 5; i++) {
        model::Message msg;
        msg.messageId = QString("msg%1").arg(i);
        msg.channelId = "channel1";
        msg.senderId = "user1";
        msg.timestamp = QDateTime::currentDateTime();
        msg.payload = QJsonObject{{"index", i}};
        cache.addMessage(msg);
    }
    
    QList<model::Message> messages = cache.getMessagesSince("msg2");
    
    QCOMPARE(messages.size(), 2);
    QCOMPARE(messages[0].messageId, QString("msg3"));
    QCOMPARE(messages[1].messageId, QString("msg4"));
}

void tst_recentmessagecache::getMessagesSinceInvalid() {
    RecentMessageCache cache;
    
    model::Message msg1;
    msg1.messageId = "msg1";
    msg1.channelId = "channel1";
    msg1.senderId = "user1";
    msg1.timestamp = QDateTime::currentDateTime();
    msg1.payload = QJsonObject();
    cache.addMessage(msg1);
    
    model::Message msg2;
    msg2.messageId = "msg2";
    msg2.channelId = "channel1";
    msg2.senderId = "user1";
    msg2.timestamp = QDateTime::currentDateTime();
    msg2.payload = QJsonObject();
    cache.addMessage(msg2);
    
    QList<model::Message> messages = cache.getMessagesSince("nonexistent");
    
    QVERIFY(messages.isEmpty());
}

void tst_recentmessagecache::getMessagesSinceEmpty() {
    RecentMessageCache cache;
    
    QList<model::Message> messages = cache.getMessagesSince("msg1");
    
    QVERIFY(messages.isEmpty());
}

void tst_recentmessagecache::getMessagesSinceLast() {
    RecentMessageCache cache;
    
    model::Message msg1;
    msg1.messageId = "msg1";
    msg1.channelId = "channel1";
    msg1.senderId = "user1";
    msg1.timestamp = QDateTime::currentDateTime();
    msg1.payload = QJsonObject();
    cache.addMessage(msg1);
    
    model::Message msg2;
    msg2.messageId = "msg2";
    msg2.channelId = "channel1";
    msg2.senderId = "user1";
    msg2.timestamp = QDateTime::currentDateTime();
    msg2.payload = QJsonObject();
    cache.addMessage(msg2);
    
    model::Message msg3;
    msg3.messageId = "msg3";
    msg3.channelId = "channel1";
    msg3.senderId = "user1";
    msg3.timestamp = QDateTime::currentDateTime();
    msg3.payload = QJsonObject();
    cache.addMessage(msg3);
    
    QList<model::Message> messages = cache.getMessagesSince("msg3");
    
    QVERIFY(messages.isEmpty());
}

// Edge cases
void tst_recentmessagecache::addEmptyMessageId() {
    RecentMessageCache cache;
    
    model::Message msg;
    msg.messageId = "";
    msg.channelId = "channel1";
    msg.senderId = "user1";
    msg.timestamp = QDateTime::currentDateTime();
    msg.payload = QJsonObject{{"test", true}};
    
    cache.addMessage(msg);
    
    QCOMPARE(cache.size(), 0);
    QVERIFY(!cache.contains(""));
}

void tst_recentmessagecache::zeroCapacity() {
    RecentMessageCache cache(0);
    
    model::Message msg;
    msg.messageId = "msg1";
    msg.channelId = "channel1";
    msg.senderId = "user1";
    msg.timestamp = QDateTime::currentDateTime();
    msg.payload = QJsonObject();
    
    cache.addMessage(msg);
    
    QCOMPARE(cache.size(), 0);
}

void tst_recentmessagecache::clearCache() {
    RecentMessageCache cache;
    
    for (int i = 0; i < 10; i++) {
        model::Message msg;
        msg.messageId = QString("msg%1").arg(i);
        msg.channelId = "channel1";
        msg.senderId = "user1";
        msg.timestamp = QDateTime::currentDateTime();
        msg.payload = QJsonObject();
        cache.addMessage(msg);
    }
    
    QCOMPARE(cache.size(), 10);
    
    cache.clear();
    
    QCOMPARE(cache.size(), 0);
    QVERIFY(cache.isEmpty());
    QVERIFY(!cache.contains("msg0"));
}

void tst_recentmessagecache::duplicateMessages() {
    RecentMessageCache cache(5);
    
    model::Message msg1;
    msg1.messageId = "msg1";
    msg1.channelId = "channel1";
    msg1.senderId = "user1";
    msg1.timestamp = QDateTime::currentDateTime();
    msg1.payload = QJsonObject{{"version", 1}};
    cache.addMessage(msg1);
    
    model::Message msg2;
    msg2.messageId = "msg2";
    msg2.channelId = "channel1";
    msg2.senderId = "user1";
    msg2.timestamp = QDateTime::currentDateTime();
    msg2.payload = QJsonObject{{"version", 1}};
    cache.addMessage(msg2);
    
    model::Message msg1Updated;
    msg1Updated.messageId = "msg1";
    msg1Updated.channelId = "channel1";
    msg1Updated.senderId = "user1";
    msg1Updated.timestamp = QDateTime::currentDateTime();
    msg1Updated.payload = QJsonObject{{"version", 2}};
    cache.addMessage(msg1Updated);
    
    QCOMPARE(cache.size(), 2);
    
    model::Message msg = cache.getMessage("msg1");
    QCOMPARE(msg.payload.value("version").toInt(), 2);
}

// Ordering
void tst_recentmessagecache::maintainInsertionOrder() {
    RecentMessageCache cache;
    
    model::Message msg3;
    msg3.messageId = "msg3";
    msg3.channelId = "channel1";
    msg3.senderId = "user1";
    msg3.timestamp = QDateTime::currentDateTime();
    msg3.payload = QJsonObject();
    cache.addMessage(msg3);
    
    model::Message msg1;
    msg1.messageId = "msg1";
    msg1.channelId = "channel1";
    msg1.senderId = "user1";
    msg1.timestamp = QDateTime::currentDateTime();
    msg1.payload = QJsonObject();
    cache.addMessage(msg1);
    
    model::Message msg2;
    msg2.messageId = "msg2";
    msg2.channelId = "channel1";
    msg2.senderId = "user1";
    msg2.timestamp = QDateTime::currentDateTime();
    msg2.payload = QJsonObject();
    cache.addMessage(msg2);
    
    QList<model::Message> messages = cache.getAllMessages();
    
    QCOMPARE(messages[0].messageId, QString("msg3"));
    QCOMPARE(messages[1].messageId, QString("msg1"));
    QCOMPARE(messages[2].messageId, QString("msg2"));
}

void tst_recentmessagecache::orderAfterEviction() {
    RecentMessageCache cache(3);
    
    model::Message msg1;
    msg1.messageId = "msg1";
    msg1.channelId = "channel1";
    msg1.senderId = "user1";
    msg1.timestamp = QDateTime::currentDateTime();
    msg1.payload = QJsonObject();
    cache.addMessage(msg1);
    
    model::Message msg2;
    msg2.messageId = "msg2";
    msg2.channelId = "channel1";
    msg2.senderId = "user1";
    msg2.timestamp = QDateTime::currentDateTime();
    msg2.payload = QJsonObject();
    cache.addMessage(msg2);
    
    model::Message msg3;
    msg3.messageId = "msg3";
    msg3.channelId = "channel1";
    msg3.senderId = "user1";
    msg3.timestamp = QDateTime::currentDateTime();
    msg3.payload = QJsonObject();
    cache.addMessage(msg3);
    
    model::Message msg4;
    msg4.messageId = "msg4";
    msg4.channelId = "channel1";
    msg4.senderId = "user1";
    msg4.timestamp = QDateTime::currentDateTime();
    msg4.payload = QJsonObject();
    cache.addMessage(msg4);
    
    QList<model::Message> messages = cache.getAllMessages();
    
    QCOMPARE(messages.size(), 3);
    QCOMPARE(messages[0].messageId, QString("msg2"));
    QCOMPARE(messages[1].messageId, QString("msg3"));
    QCOMPARE(messages[2].messageId, QString("msg4"));
}

QTEST_APPLESS_MAIN(tst_recentmessagecache)
#include "tst_recentmessagecache.moc"