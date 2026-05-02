/*!
 * @file tst_garbagecollector.cpp
 * @brief Unit tests for the GarbageCollector service
 *
 * Tests TTL-based message cleanup and batch deletion.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#include <QtTest>
#include <QSignalSpy>
#include "../appcomm/garbagecollector.h"
#include "../appcomm/appwritesdk.h"
#include "../appcomm/model.h"

using namespace appcomm::server;
using namespace appcomm::model;
using namespace appwritesdk;

class tst_garbagecollector : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Constructor tests
    void testConstructor();

    // Cleanup tests
    void testRunCleanupWithZeroTTL();
    void testRunCleanupWithNegativeTTL();
    void testRunCleanupWithValidTTL();

    // Signal tests
    void testCleanupCompleteSignal();
    void testCleanupErrorSignal();

private:
    Server *m_server;
    ConnectionConfig m_config;
};

void tst_garbagecollector::initTestCase() {
    // Setup test configuration
    m_config.endpoint = "https://cloud.appwrite.io/v1";
    m_config.projectId = "test-project-123";
    m_config.apiKey = "test-api-key";
    m_config.dbId = "test-db";
    m_config.collectionId = "messages";
    
    m_server = new Server(new QNetworkAccessManager(this), this);
}

void tst_garbagecollector::cleanupTestCase() {
    // Server will be deleted by Qt parent-child relationship
}

void tst_garbagecollector::init() {
    // Reset before each test
}

void tst_garbagecollector::cleanup() {
    // Clean up after each test
}

void tst_garbagecollector::testConstructor() {
    GarbageCollector gc(m_server, m_config);
    
    QVERIFY(true); // Constructor should not crash
}

void tst_garbagecollector::testRunCleanupWithZeroTTL() {
    GarbageCollector gc(m_server, m_config);
    
    QSignalSpy spyComplete(&gc, &GarbageCollector::cleanupComplete);
    QSignalSpy spyError(&gc, &GarbageCollector::cleanupError);
    
    PersistencePolicy policy;
    policy.messageTTL = 0; // Should skip cleanup
    
    gc.runCleanup(policy);
    
    // Should emit cleanupComplete(0) immediately
    QCOMPARE(spyComplete.count(), 1);
    QCOMPARE(spyError.count(), 0);
    
    QList<QVariant> arguments = spyComplete.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 0);
}

void tst_garbagecollector::testRunCleanupWithNegativeTTL() {
    GarbageCollector gc(m_server, m_config);
    
    QSignalSpy spyComplete(&gc, &GarbageCollector::cleanupComplete);
    
    PersistencePolicy policy;
    policy.messageTTL = -1; // Should skip cleanup
    
    gc.runCleanup(policy);
    
    // Should emit cleanupComplete(0) immediately
    QCOMPARE(spyComplete.count(), 1);
    
    QList<QVariant> arguments = spyComplete.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 0);
}

void tst_garbagecollector::testRunCleanupWithValidTTL() {
    QSKIP("Requires a mocked Appwrite server response to validate cleanup batching deterministically.");
}

void tst_garbagecollector::testCleanupCompleteSignal() {
    GarbageCollector gc(m_server, m_config);
    
    QSignalSpy spy(&gc, &GarbageCollector::cleanupComplete);
    
    QVERIFY(spy.isValid());
}

void tst_garbagecollector::testCleanupErrorSignal() {
    GarbageCollector gc(m_server, m_config);
    
    QSignalSpy spy(&gc, &GarbageCollector::cleanupError);
    
    QVERIFY(spy.isValid());
}

QTEST_MAIN(tst_garbagecollector)
#include "tst_garbagecollector.moc"
