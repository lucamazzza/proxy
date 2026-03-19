#include <QTest>
#include <QJsonObject>
#include "recentmessagecache.h"

using namespace appcomm;

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
    RecentMessageCache cache(50);
    QCOMPARE(cache.size(), 0);
    QCOMPARE(cache.capacity(), 50);
    QVERIFY(cache.isEmpty());
}

void tst_recentmessagecache::addSingleMessage() {
    RecentMessageCache cache;
    QJsonObject data{{"text", "Hello"}};
    
    cache.addMessage("msg1", data);
    
    QCOMPARE(cache.size(), 1);
    QVERIFY(!cache.isEmpty());
    QVERIFY(cache.contains("msg1"));
}

void tst_recentmessagecache::addMultipleMessages() {
    RecentMessageCache cache;
    
    for (int i = 0; i < 10; i++) {
        QJsonObject data{{"index", i}};
        cache.addMessage(QString("msg%1").arg(i), data);
    }
    
    QCOMPARE(cache.size(), 10);
    QVERIFY(cache.contains("msg0"));
    QVERIFY(cache.contains("msg9"));
}

void tst_recentmessagecache::updateExistingMessage() {
    RecentMessageCache cache;
    QJsonObject data1{{"text", "Original"}};
    QJsonObject data2{{"text", "Updated"}};
    
    cache.addMessage("msg1", data1);
    QCOMPARE(cache.size(), 1);
    
    cache.addMessage("msg1", data2);
    QCOMPARE(cache.size(), 1);
    
    CachedMessage msg = cache.getMessage("msg1");
    QCOMPARE(msg.data.value("text").toString(), QString("Updated"));
}

void tst_recentmessagecache::checkContains() {
    RecentMessageCache cache;
    QJsonObject data{{"test", true}};
    
    QVERIFY(!cache.contains("msg1"));
    
    cache.addMessage("msg1", data);
    QVERIFY(cache.contains("msg1"));
    QVERIFY(!cache.contains("msg2"));
}

void tst_recentmessagecache::getMessage() {
    RecentMessageCache cache;
    QJsonObject data{{"value", 42}};
    
    cache.addMessage("msg1", data);
    
    CachedMessage msg = cache.getMessage("msg1");
    QCOMPARE(msg.messageId, QString("msg1"));
    QCOMPARE(msg.data.value("value").toInt(), 42);
    QVERIFY(msg.timestamp > 0);
}

void tst_recentmessagecache::getAllMessages() {
    RecentMessageCache cache;
    
    for (int i = 0; i < 5; i++) {
        QJsonObject data{{"index", i}};
        cache.addMessage(QString("msg%1").arg(i), data);
    }
    
    QList<CachedMessage> messages = cache.getAllMessages();
    QCOMPARE(messages.size(), 5);
    QCOMPARE(messages[0].messageId, QString("msg0"));
    QCOMPARE(messages[4].messageId, QString("msg4"));
}

// Capacity management
void tst_recentmessagecache::respectCapacity() {
    RecentMessageCache cache(3);
    
    for (int i = 0; i < 3; i++) {
        cache.addMessage(QString("msg%1").arg(i), QJsonObject());
    }
    
    QCOMPARE(cache.size(), 3);
}

void tst_recentmessagecache::evictOldestMessage() {
    RecentMessageCache cache(3);
    
    cache.addMessage("msg0", QJsonObject{{"value", 0}});
    cache.addMessage("msg1", QJsonObject{{"value", 1}});
    cache.addMessage("msg2", QJsonObject{{"value", 2}});
    QVERIFY(cache.contains("msg0"));
    
    cache.addMessage("msg3", QJsonObject{{"value", 3}});
    
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
        cache.addMessage(QString("msg%1").arg(i), QJsonObject{{"index", i}});
    }
    
    QList<CachedMessage> messages = cache.getMessagesSince("msg2");
    
    QCOMPARE(messages.size(), 2);
    QCOMPARE(messages[0].messageId, QString("msg3"));
    QCOMPARE(messages[1].messageId, QString("msg4"));
}

void tst_recentmessagecache::getMessagesSinceInvalid() {
    RecentMessageCache cache;
    
    cache.addMessage("msg1", QJsonObject());
    cache.addMessage("msg2", QJsonObject());
    
    QList<CachedMessage> messages = cache.getMessagesSince("nonexistent");
    
    QVERIFY(messages.isEmpty());
}

void tst_recentmessagecache::getMessagesSinceEmpty() {
    RecentMessageCache cache;
    
    QList<CachedMessage> messages = cache.getMessagesSince("msg1");
    
    QVERIFY(messages.isEmpty());
}

void tst_recentmessagecache::getMessagesSinceLast() {
    RecentMessageCache cache;
    
    cache.addMessage("msg1", QJsonObject());
    cache.addMessage("msg2", QJsonObject());
    cache.addMessage("msg3", QJsonObject());
    
    QList<CachedMessage> messages = cache.getMessagesSince("msg3");
    
    QVERIFY(messages.isEmpty());
}

// Edge cases
void tst_recentmessagecache::addEmptyMessageId() {
    RecentMessageCache cache;
    
    cache.addMessage("", QJsonObject{{"test", true}});
    
    QCOMPARE(cache.size(), 0);
    QVERIFY(!cache.contains(""));
}

void tst_recentmessagecache::zeroCapacity() {
    RecentMessageCache cache(0);
    
    cache.addMessage("msg1", QJsonObject());
    
    QCOMPARE(cache.size(), 0);
}

void tst_recentmessagecache::clearCache() {
    RecentMessageCache cache;
    
    for (int i = 0; i < 10; i++) {
        cache.addMessage(QString("msg%1").arg(i), QJsonObject());
    }
    
    QCOMPARE(cache.size(), 10);
    
    cache.clear();
    
    QCOMPARE(cache.size(), 0);
    QVERIFY(cache.isEmpty());
    QVERIFY(!cache.contains("msg0"));
}

void tst_recentmessagecache::duplicateMessages() {
    RecentMessageCache cache(5);
    
    cache.addMessage("msg1", QJsonObject{{"version", 1}});
    cache.addMessage("msg2", QJsonObject{{"version", 1}});
    cache.addMessage("msg1", QJsonObject{{"version", 2}});
    
    QCOMPARE(cache.size(), 2);
    
    CachedMessage msg = cache.getMessage("msg1");
    QCOMPARE(msg.data.value("version").toInt(), 2);
}

// Ordering
void tst_recentmessagecache::maintainInsertionOrder() {
    RecentMessageCache cache;
    
    cache.addMessage("msg3", QJsonObject());
    cache.addMessage("msg1", QJsonObject());
    cache.addMessage("msg2", QJsonObject());
    
    QList<CachedMessage> messages = cache.getAllMessages();
    
    QCOMPARE(messages[0].messageId, QString("msg3"));
    QCOMPARE(messages[1].messageId, QString("msg1"));
    QCOMPARE(messages[2].messageId, QString("msg2"));
}

void tst_recentmessagecache::orderAfterEviction() {
    RecentMessageCache cache(3);
    
    cache.addMessage("msg1", QJsonObject());
    cache.addMessage("msg2", QJsonObject());
    cache.addMessage("msg3", QJsonObject());
    cache.addMessage("msg4", QJsonObject());
    
    QList<CachedMessage> messages = cache.getAllMessages();
    
    QCOMPARE(messages.size(), 3);
    QCOMPARE(messages[0].messageId, QString("msg2"));
    QCOMPARE(messages[1].messageId, QString("msg3"));
    QCOMPARE(messages[2].messageId, QString("msg4"));
}

QTEST_APPLESS_MAIN(tst_recentmessagecache)
#include "tst_recentmessagecache.moc"