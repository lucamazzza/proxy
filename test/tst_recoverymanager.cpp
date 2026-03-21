#include <QTest>
#include <QSignalSpy>
#include <QJsonObject>
#include <QJsonArray>
#include "recoverymanager.h"
#include "appwritesdk.h"
#include "recentmessagecache.h"

using namespace appcomm;
using namespace AppwriteSDK;

class MockClient : public Client {
    Q_OBJECT
public:
    using Client::Client;
    
    void simulateSuccess(const QJsonObject &data) {
        emit requestSuccess(data);
    }
    
    void simulateError(int code, const QString &message) {
        emit requestError(code, message);
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
    m_networkManager = new QNetworkAccessManager(this);
    m_client = new MockClient(m_networkManager, this);
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
    delete m_networkManager;
    
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
    m_cache->addMessage("msg1", QJsonObject{{"text", "First"}});
    m_cache->addMessage("msg2", QJsonObject{{"text", "Second"}});
    m_cache->addMessage("msg3", QJsonObject{{"text", "Third"}});
    
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->requestFrom("msg1");
    
    QCOMPARE(spy.count(), 1);
    QJsonArray messages = spy.at(0).at(0).toJsonArray();
    QCOMPARE(messages.size(), 2);
    QCOMPARE(messages[0].toObject()["$id"].toString(), QString("msg2"));
    QCOMPARE(messages[1].toObject()["$id"].toString(), QString("msg3"));
}

void tst_recoverymanager::requestFromCacheHitMultiple()
{
    for (int i = 0; i < 10; i++) {
        m_cache->addMessage(QString("msg%1").arg(i), QJsonObject{{"index", i}});
    }
    
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->requestFrom("msg4");
    
    QCOMPARE(spy.count(), 1);
    QJsonArray messages = spy.at(0).at(0).toJsonArray();
    QCOMPARE(messages.size(), 5);
    QCOMPARE(messages[0].toObject()["$id"].toString(), QString("msg5"));
    QCOMPARE(messages[4].toObject()["$id"].toString(), QString("msg9"));
}

void tst_recoverymanager::requestFromCacheMiss()
{
    m_cache->addMessage("msg1", QJsonObject{{"text", "First"}});
    
    QSignalSpy errorSpy(m_manager, &RecoveryManager::recoveryError);
    
    m_manager->requestFrom("nonexistent");
    
    QCOMPARE(errorSpy.count(), 1);
}

void tst_recoverymanager::requestSingleCacheHit()
{
    m_cache->addMessage("msg1", QJsonObject{{"text", "Hello"}});
    
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->request("msg1");
    
    QCOMPARE(spy.count(), 1);
    QJsonArray messages = spy.at(0).at(0).toJsonArray();
    QCOMPARE(messages.size(), 1);
    QCOMPARE(messages[0].toObject()["$id"].toString(), QString("msg1"));
    QCOMPARE(messages[0].toObject()["data"].toObject()["text"].toString(), QString("Hello"));
}

void tst_recoverymanager::requestSingleCacheMiss()
{
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->request("msg1");
    
    QJsonObject serverResponse;
    serverResponse["$id"] = "msg1";
    serverResponse["text"] = "From server";
    
    m_client->simulateSuccess(serverResponse);
    
    QCOMPARE(spy.count(), 1);
    QJsonArray messages = spy.at(0).at(0).toJsonArray();
    QCOMPARE(messages.size(), 1);
    QCOMPARE(messages[0].toObject()["$id"].toString(), QString("msg1"));
}

void tst_recoverymanager::requestFullResync()
{
    m_cache->addMessage("old1", QJsonObject{{"old", true}});
    m_cache->addMessage("old2", QJsonObject{{"old", true}});
    
    QSignalSpy recoverSpy(m_manager, &RecoveryManager::messagesRecovered);
    QSignalSpy resyncSpy(m_manager, &RecoveryManager::resyncCompleted);
    
    m_manager->requestFullResync();
    
    QVERIFY(m_cache->isEmpty());
    
    QJsonObject serverResponse;
    QJsonArray documents;
    for (int i = 0; i < 5; i++) {
        QJsonObject doc;
        doc["$id"] = QString("msg%1").arg(i);
        doc["text"] = QString("Message %1").arg(i);
        documents.append(doc);
    }
    serverResponse["documents"] = documents;
    
    m_client->simulateSuccess(serverResponse);
    
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
    
    m_client->simulateError(404, "Document not found");
    
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.at(0).at(0).toInt(), 404);
    QCOMPARE(errorSpy.at(0).at(1).toString(), QString("Document not found"));
}

void tst_recoverymanager::stateAlignedAfterRequestFrom()
{
    m_cache->addMessage("msg1", QJsonObject{{"version", 1}});
    
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->requestFrom("msg1");
    
    QJsonObject serverResponse;
    QJsonArray documents;
    QJsonObject doc1;
    doc1["$id"] = "msg2";
    doc1["text"] = "New message";
    documents.append(doc1);
    serverResponse["documents"] = documents;
    
    m_client->simulateSuccess(serverResponse);
    
    QCOMPARE(spy.count(), 1);
    QVERIFY(m_cache->contains("msg2"));
}

void tst_recoverymanager::stateAlignedAfterRequest()
{
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->request("msg1");
    
    QJsonObject serverResponse;
    serverResponse["$id"] = "msg1";
    serverResponse["text"] = "Retrieved message";
    
    m_client->simulateSuccess(serverResponse);
    
    QVERIFY(m_cache->contains("msg1"));
    CachedMessage msg = m_cache->getMessage("msg1");
    QCOMPARE(msg.messageId, QString("msg1"));
}

void tst_recoverymanager::stateAlignedAfterFullResync()
{
    m_cache->addMessage("old1", QJsonObject{{"old", true}});
    
    m_manager->requestFullResync();
    
    QJsonObject serverResponse;
    QJsonArray documents;
    for (int i = 0; i < 3; i++) {
        QJsonObject doc;
        doc["$id"] = QString("new%1").arg(i);
        documents.append(doc);
    }
    serverResponse["documents"] = documents;
    
    m_client->simulateSuccess(serverResponse);
    
    QVERIFY(!m_cache->contains("old1"));
    QVERIFY(m_cache->contains("new0"));
    QVERIFY(m_cache->contains("new1"));
    QVERIFY(m_cache->contains("new2"));
    QCOMPARE(m_cache->size(), 3);
}

void tst_recoverymanager::requestFromLastMessage()
{
    m_cache->addMessage("msg1", QJsonObject());
    m_cache->addMessage("msg2", QJsonObject());
    
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->requestFrom("msg2");
    
    QJsonObject serverResponse;
    QJsonArray documents;
    serverResponse["documents"] = documents;
    
    m_client->simulateSuccess(serverResponse);
    
    QCOMPARE(spy.count(), 1);
    QJsonArray messages = spy.at(0).at(0).toJsonArray();
    QVERIFY(messages.isEmpty());
}

void tst_recoverymanager::fullResyncClearsCache()
{
    m_cache->addMessage("msg1", QJsonObject());
    m_cache->addMessage("msg2", QJsonObject());
    
    QCOMPARE(m_cache->size(), 2);
    
    m_manager->requestFullResync();
    
    QCOMPARE(m_cache->size(), 0);
}

void tst_recoverymanager::multipleSequentialRequests()
{
    QSignalSpy spy(m_manager, &RecoveryManager::messagesRecovered);
    
    m_manager->request("msg1");
    QJsonObject response1;
    response1["$id"] = "msg1";
    m_client->simulateSuccess(response1);
    
    m_manager->request("msg2");
    QJsonObject response2;
    response2["$id"] = "msg2";
    m_client->simulateSuccess(response2);
    
    QCOMPARE(spy.count(), 2);
    QVERIFY(m_cache->contains("msg1"));
    QVERIFY(m_cache->contains("msg2"));
}

QTEST_MAIN(tst_recoverymanager)
#include "tst_recoverymanager.moc"
