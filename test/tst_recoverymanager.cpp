#include <QTest>
#include <QSignalSpy>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include "recoverymanager.h"
#include "appwritesdk.h"
#include "recentmessagecache.h"
#include "model.h"

using namespace appcomm;
using namespace appcomm::client;
using namespace appwritesdk;

class MockClient : public Client {
    Q_OBJECT
public:
    using Client::Client;
    
    void simulateSuccess(appwritesdk::RequestType type, const QJsonObject &data) {
        emit requestSuccess(type, data);
    }

    void simulateError(appwritesdk::RequestType type, int code, const QString &message) {
        emit requestError(type, code, message);
    }
};

class tst_recoverymanager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality
    void construction();
    void requestFromCacheHit();
    void requestFromCacheHitMultiple();
    void requestFromCacheMiss();
    void requestSingleCacheHit();
    void requestSingleCacheMiss();
    void requestFullResync();

    // Error handling
    void requestFromEmptyMessageId();
    void requestEmptyMessageId();
    void requestFromInvalidMessageId();
    void serverError();

    // State alignment
    void stateAlignedAfterRequestFrom();
    void stateAlignedAfterRequest();
    void stateAlignedAfterFullResync();

    // Edge cases
    void requestFromLastMessage();
    void fullResyncClearsCache();
    void multipleSequentialRequests();

private:
    QNetworkAccessManager *m_networkManager;
    MockClient *m_client;
    RecentMessageCache *m_cache;
    ConnectionConfig m_config;
    RecoveryManager *m_manager;
};

void tst_recoverymanager::initTestCase()
{
    m_networkManager = nullptr;
    m_client = nullptr;
    m_cache = nullptr;
    m_manager = nullptr;
}

void tst_recoverymanager::cleanupTestCase() {}

void tst_recoverymanager::init()
{
    //m_networkManager = new QNetworkAccessManager(this);
    m_client = new MockClient(this);
    m_cache = new RecentMessageCache(100, this);
    
    m_config.endpoint = "http://localhost/v1";
    m_config.projectId = "test-project";
    m_config.apiKey = "test-key";
    m_config.dbId = "test-db";
    m_config.collectionId = "messages";
    
    m_manager = new RecoveryManager(m_client, m_cache, m_config, this);
}

void tst_recoverymanager::cleanup()
{
    delete m_manager;
    delete m_cache;
    delete m_client;
    //delete m_networkManager;
    
    m_manager = nullptr;
    m_cache = nullptr;
    m_client = nullptr;
    m_networkManager = nullptr;
}

void tst_recoverymanager::construction()
{
    QVERIFY(m_manager != nullptr);
}

void tst_recoverymanager::requestFromCacheHit()
{
    model::Message msg1;
    msg1.messageId = "msg1";
    msg1.channelId = "channel1";
    msg1.senderId = "user1";
    msg1.timestamp = QDateTime::currentDateTime();
    msg1.payload = QJsonObject{{"text", "First"}};
    m_cache->addMessage(msg1);
    
    model::Message msg2;
    msg2.messageId = "msg2";
    msg2.channelId = "channel1";
    msg2.senderId = "user1";
    msg2.timestamp = QDateTime::currentDateTime();
    msg2.payload = QJsonObject{{"text", "Second"}};
    m_cache->addMessage(msg2);
    
    model::Message msg3;
    msg3.messageId = "msg3";
    msg3.channelId = "channel1";
    msg3.senderId = "user1";
    msg3.timestamp = QDateTime::currentDateTime();
    msg3.payload = QJsonObject{{"text", "Third"}};
    m_cache->addMessage(msg3);
    
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->requestFrom("msg1");
    
    QCOMPARE(spy.count(), 1);
    QJsonArray messages = spy.at(0).at(0).toJsonArray();
    QCOMPARE(messages.size(), 2);
    QCOMPARE(messages[0].toObject()["messageId"].toString(), QString("msg2"));
    QCOMPARE(messages[1].toObject()["messageId"].toString(), QString("msg3"));
}

void tst_recoverymanager::requestFromCacheHitMultiple()
{
    for (int i = 0; i < 10; i++) {
        model::Message msg;
        msg.messageId = QString("msg%1").arg(i);
        msg.channelId = "channel1";
        msg.senderId = "user1";
        msg.timestamp = QDateTime::currentDateTime();
        msg.payload = QJsonObject{{"index", i}};
        m_cache->addMessage(msg);
    }
    
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->requestFrom("msg4");
    
    QCOMPARE(spy.count(), 1);
    QJsonArray messages = spy.at(0).at(0).toJsonArray();
    QCOMPARE(messages.size(), 5);
    QCOMPARE(messages[0].toObject()["messageId"].toString(), QString("msg5"));
    QCOMPARE(messages[4].toObject()["messageId"].toString(), QString("msg9"));
}

void tst_recoverymanager::requestFromCacheMiss()
{
    model::Message msg;
    msg.messageId = "msg1";
    msg.channelId = "channel1";
    msg.senderId = "user1";
    msg.timestamp = QDateTime::currentDateTime();
    msg.payload = QJsonObject{{"text", "First"}};
    m_cache->addMessage(msg);
    
    QSignalSpy errorSpy(m_manager, &RecoveryManager::recoveryError);
    
    m_manager->requestFrom("nonexistent");
    
    QCOMPARE(errorSpy.count(), 1);
}

void tst_recoverymanager::requestSingleCacheHit()
{
    model::Message msg;
    msg.messageId = "msg1";
    msg.channelId = "channel1";
    msg.senderId = "user1";
    msg.timestamp = QDateTime::currentDateTime();
    msg.payload = QJsonObject{{"text", "Hello"}};
    m_cache->addMessage(msg);
    
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->request("msg1");
    
    QCOMPARE(spy.count(), 1);
    QJsonArray messages = spy.at(0).at(0).toJsonArray();
    QCOMPARE(messages.size(), 1);
    QCOMPARE(messages[0].toObject()["messageId"].toString(), QString("msg1"));
}

void tst_recoverymanager::requestSingleCacheMiss()
{
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->request("msg1");
    
    QJsonObject serverResponse;
    serverResponse["messageId"] = "msg1";
    serverResponse["channelId"] = "channel1";
    serverResponse["senderId"] = "user1";
    serverResponse["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    serverResponse["payload"] = QJsonObject{{"text", "From server"}};
    serverResponse["isEcho"] = false;
    
    m_client->simulateSuccess(appwritesdk::RequestType::GetDocument, serverResponse);
    
    QCOMPARE(spy.count(), 1);
    QJsonArray messages = spy.at(0).at(0).toJsonArray();
    QCOMPARE(messages.size(), 1);
}

void tst_recoverymanager::requestFullResync()
{
    model::Message msg1;
    msg1.messageId = "old1";
    msg1.channelId = "channel1";
    msg1.senderId = "user1";
    msg1.timestamp = QDateTime::currentDateTime();
    msg1.payload = QJsonObject{{"old", true}};
    m_cache->addMessage(msg1);
    
    model::Message msg2;
    msg2.messageId = "old2";
    msg2.channelId = "channel1";
    msg2.senderId = "user1";
    msg2.timestamp = QDateTime::currentDateTime();
    msg2.payload = QJsonObject{{"old", true}};
    m_cache->addMessage(msg2);
    
    QSignalSpy recoverSpy(m_manager, &RecoveryManager::messagesRecovered);
    QSignalSpy resyncSpy(m_manager, &RecoveryManager::resyncCompleted);
    
    m_manager->requestFullResync();
    
    QVERIFY(m_cache->isEmpty());
    
    QJsonObject serverResponse;
    QJsonArray documents;
    for (int i = 0; i < 5; i++) {
        QJsonObject doc;
        doc["messageId"] = QString("msg%1").arg(i);
        doc["channelId"] = "channel1";
        doc["senderId"] = "user1";
        doc["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        doc["payload"] = QJsonObject{{"text", QString("Message %1").arg(i)}};
        doc["isEcho"] = false;
        documents.append(doc);
    }
    serverResponse["documents"] = documents;
    
    m_client->simulateSuccess(appwritesdk::RequestType::ListDocuments, serverResponse);
    
    QCOMPARE(recoverSpy.count(), 1);
    QCOMPARE(resyncSpy.count(), 1);
    QCOMPARE(resyncSpy.at(0).at(0).toInt(), 5);
    
    QCOMPARE(m_cache->size(), 5);
    QVERIFY(m_cache->contains("msg0"));
    QVERIFY(m_cache->contains("msg4"));
}

void tst_recoverymanager::requestFromEmptyMessageId()
{
    QSignalSpy errorSpy(m_manager, &RecoveryManager::recoveryError);
    
    m_manager->requestFrom("");
    
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.at(0).at(0).toInt(), -1);
}

void tst_recoverymanager::requestEmptyMessageId()
{
    QSignalSpy errorSpy(m_manager, &RecoveryManager::recoveryError);
    
    m_manager->request("");
    
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.at(0).at(0).toInt(), -1);
}

void tst_recoverymanager::requestFromInvalidMessageId()
{
    QSignalSpy errorSpy(m_manager, &RecoveryManager::recoveryError);
    
    m_manager->requestFrom("invalid-id");
    
    QCOMPARE(errorSpy.count(), 1);
}

void tst_recoverymanager::serverError()
{
    QSignalSpy errorSpy(m_manager, &RecoveryManager::recoveryError);
    
    m_manager->request("msg1");
    
    m_client->simulateError(appwritesdk::RequestType::GetDocument, 404, "Document not found");
    
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.at(0).at(0).toInt(), 404);
    QCOMPARE(errorSpy.at(0).at(1).toString(), QString("Document not found"));
}

void tst_recoverymanager::stateAlignedAfterRequestFrom()
{
    model::Message msg;
    msg.messageId = "msg1";
    msg.channelId = "channel1";
    msg.senderId = "user1";
    msg.timestamp = QDateTime::currentDateTime();
    msg.payload = QJsonObject{{"version", 1}};
    m_cache->addMessage(msg);
    
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->requestFrom("msg1");
    
    QJsonObject serverResponse;
    QJsonArray documents;
    QJsonObject doc1;
    doc1["messageId"] = "msg2";
    doc1["channelId"] = "channel1";
    doc1["senderId"] = "user1";
    doc1["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    doc1["payload"] = QJsonObject{{"text", "New message"}};
    doc1["isEcho"] = false;
    documents.append(doc1);
    serverResponse["documents"] = documents;
    
    m_client->simulateSuccess(appwritesdk::RequestType::ListDocuments, serverResponse);
    
    QCOMPARE(spy.count(), 1);
    QVERIFY(m_cache->contains("msg2"));
}

void tst_recoverymanager::stateAlignedAfterRequest()
{
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->request("msg1");
    
    QJsonObject serverResponse;
    serverResponse["messageId"] = "msg1";
    serverResponse["channelId"] = "channel1";
    serverResponse["senderId"] = "user1";
    serverResponse["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    serverResponse["payload"] = QJsonObject{{"text", "Retrieved message"}};
    serverResponse["isEcho"] = false;
    
    m_client->simulateSuccess(appwritesdk::RequestType::GetDocument, serverResponse);
    
    QVERIFY(m_cache->contains("msg1"));
    model::Message msg = m_cache->getMessage("msg1");
    QCOMPARE(msg.messageId, QString("msg1"));
}

void tst_recoverymanager::stateAlignedAfterFullResync()
{
    model::Message msg;
    msg.messageId = "old1";
    msg.channelId = "channel1";
    msg.senderId = "user1";
    msg.timestamp = QDateTime::currentDateTime();
    msg.payload = QJsonObject{{"old", true}};
    m_cache->addMessage(msg);
    
    m_manager->requestFullResync();
    
    QJsonObject serverResponse;
    QJsonArray documents;
    for (int i = 0; i < 3; i++) {
        QJsonObject doc;
        doc["messageId"] = QString("new%1").arg(i);
        doc["channelId"] = "channel1";
        doc["senderId"] = "user1";
        doc["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        doc["payload"] = QJsonObject();
        doc["isEcho"] = false;
        documents.append(doc);
    }
    serverResponse["documents"] = documents;
    
    m_client->simulateSuccess(appwritesdk::RequestType::ListDocuments, serverResponse);
    
    QVERIFY(!m_cache->contains("old1"));
    QVERIFY(m_cache->contains("new0"));
    QVERIFY(m_cache->contains("new1"));
    QVERIFY(m_cache->contains("new2"));
    QCOMPARE(m_cache->size(), 3);
}

void tst_recoverymanager::requestFromLastMessage()
{
    model::Message msg1;
    msg1.messageId = "msg1";
    msg1.channelId = "channel1";
    msg1.senderId = "user1";
    msg1.timestamp = QDateTime::currentDateTime();
    msg1.payload = QJsonObject();
    m_cache->addMessage(msg1);
    
    model::Message msg2;
    msg2.messageId = "msg2";
    msg2.channelId = "channel1";
    msg2.senderId = "user1";
    msg2.timestamp = QDateTime::currentDateTime();
    msg2.payload = QJsonObject();
    m_cache->addMessage(msg2);
    
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->requestFrom("msg2");
    
    QJsonObject serverResponse;
    QJsonArray documents;
    serverResponse["documents"] = documents;
    
    m_client->simulateSuccess(appwritesdk::RequestType::ListDocuments, serverResponse);
    
    QCOMPARE(spy.count(), 1);
    QJsonArray messages = spy.at(0).at(0).toJsonArray();
    QVERIFY(messages.isEmpty());
}

void tst_recoverymanager::fullResyncClearsCache()
{
    model::Message msg1;
    msg1.messageId = "msg1";
    msg1.channelId = "channel1";
    msg1.senderId = "user1";
    msg1.timestamp = QDateTime::currentDateTime();
    msg1.payload = QJsonObject();
    m_cache->addMessage(msg1);
    
    model::Message msg2;
    msg2.messageId = "msg2";
    msg2.channelId = "channel1";
    msg2.senderId = "user1";
    msg2.timestamp = QDateTime::currentDateTime();
    msg2.payload = QJsonObject();
    m_cache->addMessage(msg2);
    
    QCOMPARE(m_cache->size(), 2);
    
    m_manager->requestFullResync();
    
    QCOMPARE(m_cache->size(), 0);
}

void tst_recoverymanager::multipleSequentialRequests()
{
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->request("msg1");
    QJsonObject response1;
    response1["messageId"] = "msg1";
    response1["channelId"] = "channel1";
    response1["senderId"] = "user1";
    response1["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    response1["payload"] = QJsonObject();
    response1["isEcho"] = false;
    m_client->simulateSuccess(appwritesdk::RequestType::GetDocument, response1);
    
    m_manager->request("msg2");
    QJsonObject response2;
    response2["messageId"] = "msg2";
    response2["channelId"] = "channel1";
    response2["senderId"] = "user1";
    response2["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    response2["payload"] = QJsonObject();
    response2["isEcho"] = false;
    m_client->simulateSuccess(appwritesdk::RequestType::GetDocument, response2);
    
    QCOMPARE(spy.count(), 2);
    QVERIFY(m_cache->contains("msg1"));
    QVERIFY(m_cache->contains("msg2"));
}

QTEST_MAIN(tst_recoverymanager)
#include "tst_recoverymanager.moc"
