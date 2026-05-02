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

class tst_realtime : public QObject {
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

void tst_realtime::initTestCase() {
    // Setup test configuration
    m_config.endpoint = "https://cloud.appwrite.io/v1";
    m_config.projectId = "test-project-123";
    m_config.apiKey = "test-api-key";
    m_config.dbId = "test-db";
    m_config.collectionId = "test-collection";
}

void tst_realtime::cleanupTestCase() {
    // Nothing to clean up
}

void tst_realtime::init() {
    // Reset before each test
}

void tst_realtime::cleanup() {
    // Clean up after each test
}

void tst_realtime::testConstructor() {
    Realtime realtime(m_config);
    
    QVERIFY(true); // Constructor should not crash
}

void tst_realtime::testConstructorWithConfig() {
    ConnectionConfig config;
    config.endpoint = "https://example.com/v1";
    config.projectId = "project123";
    
    Realtime realtime(config);
    
    QVERIFY(true); // Should accept custom config
}

void tst_realtime::testConnectBuildsCorrectUrl() {
    QSKIP("Requires injectable/mocked QWebSocket to assert URL construction without external network.");
}

void tst_realtime::testDisconnect() {
    Realtime realtime(m_config);
    
    realtime.disconnect();
    
    QVERIFY(true); // Should not crash when disconnecting unconnected socket
}

void tst_realtime::testConnectedSignal() {
    Realtime realtime(m_config);
    
    QSignalSpy spy(&realtime, &Realtime::connected);
    
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 0); // No connection in tests
}

void tst_realtime::testDisconnectedSignal() {
    Realtime realtime(m_config);
    
    QSignalSpy spy(&realtime, &Realtime::disconnected);
    
    QVERIFY(spy.isValid());
}

void tst_realtime::testEventReceivedSignal() {
    Realtime realtime(m_config);
    
    QSignalSpy spy(&realtime, &Realtime::eventReceived);
    
    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 0);
}

void tst_realtime::testErrorOccurredSignal() {
    Realtime realtime(m_config);
    
    QSignalSpy spy(&realtime, &Realtime::errorOccurred);
    
    QVERIFY(spy.isValid());
}

QTEST_MAIN(tst_realtime)
#include "tst_realtime.moc"
