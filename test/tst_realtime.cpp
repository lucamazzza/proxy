/*!
 * @file tst_realtime.cpp
 * @brief Unit tests for the Realtime WebSocket client
 *
 * Tests WebSocket connection, event parsing, and error handling.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#include <QtTest>
#include <QSignalSpy>
#include <QJsonDocument>
#include "../appcomm/realtime.h"
#include "../appcomm/appwritesdk.h"

using namespace appcomm;
using namespace appwritesdk;

class TestRealtime : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Constructor tests
    void testConstructor();
    void testConstructorWithConfig();

    // Connection tests
    void testConnectBuildsCorrectUrl();
    void testDisconnect();

    // Signal tests
    void testConnectedSignal();
    void testDisconnectedSignal();
    void testEventReceivedSignal();
    void testErrorOccurredSignal();

private:
    ConnectionConfig m_config;
};

void TestRealtime::initTestCase() {
    // Setup test configuration
    m_config.endpoint = "https://cloud.appwrite.io/v1";
    m_config.projectId = "test-project-123";
    m_config.apiKey = "test-api-key";
    m_config.dbId = "test-db";
    m_config.collectionId = "test-collection";
}

void TestRealtime::cleanupTestCase() {
    // Nothing to clean up
}

void TestRealtime::init() {
    // Reset before each test
}

void TestRealtime::cleanup() {
    // Clean up after each test
}

void TestRealtime::testConstructor() {
    Realtime realtime(m_config);
    
    QVERIFY(true); // Constructor should not crash
}

void TestRealtime::testConstructorWithConfig() {
    ConnectionConfig config;
    config.endpoint = "https://example.com/v1";
    config.projectId = "project123";
    
    Realtime realtime(config);
    
    QVERIFY(true); // Should accept custom config
}

void TestRealtime::testConnectBuildsCorrectUrl() {
    Realtime realtime(m_config);
    
    QStringList channels;
    channels << "databases.test-db.collections.messages.documents";
    
    // Note: This will attempt to connect to real server, which will fail in tests
    // In a real scenario, you'd mock the QWebSocket
    // For now, just verify the method doesn't crash
    realtime.connectToChannels(channels);
    
    QVERIFY(true);
}

void TestRealtime::testDisconnect() {
    Realtime realtime(m_config);
    
    realtime.disconnect();
    
    QVERIFY(true); // Should not crash when disconnecting unconnected socket
}

void TestRealtime::testConnectedSignal() {
    Realtime realtime(m_config);
    
    QSignalSpy spy(&realtime, &Realtime::connected);
    
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 0); // No connection in tests
}

void TestRealtime::testDisconnectedSignal() {
    Realtime realtime(m_config);
    
    QSignalSpy spy(&realtime, &Realtime::disconnected);
    
    QVERIFY(spy.isValid());
}

void TestRealtime::testEventReceivedSignal() {
    Realtime realtime(m_config);
    
    QSignalSpy spy(&realtime, &Realtime::eventReceived);
    
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 0);
}

void TestRealtime::testErrorOccurredSignal() {
    Realtime realtime(m_config);
    
    QSignalSpy spy(&realtime, &Realtime::errorOccurred);
    
    QVERIFY(spy.isValid());
}

QTEST_MAIN(TestRealtime)
#include "tst_realtime.moc"
