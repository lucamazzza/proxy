#include <QTest>
#include "ratelimiter.h"

using namespace appcomm::client;

class tst_ratelimiter: public QObject
{
    Q_OBJECT

public:
    tst_ratelimiter();
    ~tst_ratelimiter();

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic
    void initState();
    void singleRequest();
    void bucketExhausted();
    void bucketReset();

    // Refill
    void timelyRefill();
    void partialRefill();
    void wontOverfill();
    void positiveTime();
    void accumulation();

    // Edge
    void zeroCapacity();
    void highRefillRate();
    void longIdle();
    void rapidConsecutiveCalls();

    // Getters
    void testMaxTokens();
    void testRefillRate();
    void testAvailableTokens();
};

tst_ratelimiter::tst_ratelimiter() {}
tst_ratelimiter::~tst_ratelimiter() {}
void tst_ratelimiter::initTestCase() {}
void tst_ratelimiter::cleanupTestCase() {}

// Basic
void tst_ratelimiter::initState() {
    RateLimiter limiter(10, 5);
    QCOMPARE(limiter.availableTokens(), 10);
    QCOMPARE(limiter.maxTokens(), 10);
    QCOMPARE(limiter.refillRate(), 5);
}

void tst_ratelimiter::singleRequest() {
    RateLimiter limiter(10, 5);
    QVERIFY(limiter.allowRequest());
    QCOMPARE(limiter.availableTokens(), 9);
}

void tst_ratelimiter::bucketExhausted() {
    RateLimiter limiter(3, 1);
    QVERIFY(limiter.allowRequest());
    QVERIFY(limiter.allowRequest());
    QVERIFY(limiter.allowRequest());
    QVERIFY(!limiter.allowRequest());
    QCOMPARE(limiter.availableTokens(), 0);
}

void tst_ratelimiter::bucketReset() {
    RateLimiter limiter(5, 2);
    limiter.allowRequest();
    limiter.allowRequest();
    limiter.allowRequest();
    QCOMPARE(limiter.availableTokens(), 2);
    limiter.reset();
    QCOMPARE(limiter.availableTokens(), 5);
}

// Refill
void tst_ratelimiter::timelyRefill() {
    RateLimiter limiter(10, 5);
    for (int i = 0; i < 10; i++) limiter.allowRequest();
    QCOMPARE(limiter.availableTokens(), 0);
    
    QTest::qWait(1000);
    QVERIFY(limiter.allowRequest());
    QCOMPARE(limiter.availableTokens(), 4);
}

void tst_ratelimiter::partialRefill() {
    RateLimiter limiter(10, 4);
    for (int i = 0; i < 10; i++) limiter.allowRequest();
    
    QTest::qWait(500);
    QVERIFY(limiter.allowRequest());
    QVERIFY(limiter.allowRequest());
    QVERIFY(!limiter.allowRequest());
}

void tst_ratelimiter::wontOverfill() {
    RateLimiter limiter(10, 5);
    QCOMPARE(limiter.availableTokens(), 10);
    
    QTest::qWait(2000);
    QCOMPARE(limiter.availableTokens(), 10);
}

void tst_ratelimiter::positiveTime() {
    RateLimiter limiter(10, 5);
    limiter.allowRequest();
    int before = limiter.availableTokens();
    
    QVERIFY(limiter.allowRequest());
    QVERIFY(limiter.availableTokens() <= before);
}

void tst_ratelimiter::accumulation() {
    RateLimiter limiter(10, 10);
    for (int i = 0; i < 10; i++) limiter.allowRequest();
    
    for (int i = 0; i < 5; i++) {
        QTest::qWait(100);
        limiter.allowRequest();
    }
    
    QVERIFY(limiter.availableTokens() >= 0);
}

// Edge
void tst_ratelimiter::zeroCapacity() {
    RateLimiter limiter(0, 5);
    QVERIFY(!limiter.allowRequest());
    QCOMPARE(limiter.availableTokens(), 0);
}

void tst_ratelimiter::highRefillRate() {
    RateLimiter limiter(5, 100);
    for (int i = 0; i < 5; i++) limiter.allowRequest();
    QCOMPARE(limiter.availableTokens(), 0);
    
    QTest::qWait(150);
    limiter.allowRequest();
    QVERIFY(limiter.availableTokens() >= 4);
}

void tst_ratelimiter::longIdle() {
    RateLimiter limiter(10, 10);
    for (int i = 0; i < 10; i++) limiter.allowRequest();
    QCOMPARE(limiter.availableTokens(), 0);
    
    QTest::qWait(1000);
    limiter.allowRequest();
    QVERIFY(limiter.availableTokens() >= 9);
}

void tst_ratelimiter::rapidConsecutiveCalls() {
    RateLimiter limiter(100, 50);
    for (int i = 0; i < 50; i++) {
        limiter.allowRequest();
    }
    QCOMPARE(limiter.availableTokens(), 50);
}

// Getters
void tst_ratelimiter::testMaxTokens() {
    RateLimiter limiter(42, 10);
    QCOMPARE(limiter.maxTokens(), 42);
}

void tst_ratelimiter::testRefillRate() {
    RateLimiter limiter(10, 17);
    QCOMPARE(limiter.refillRate(), 17);
}

void tst_ratelimiter::testAvailableTokens() {
    RateLimiter limiter(10, 5);
    QCOMPARE(limiter.availableTokens(), 10);
    limiter.allowRequest();
    QCOMPARE(limiter.availableTokens(), 9);
}

QTEST_APPLESS_MAIN(tst_ratelimiter)
#include "tst_ratelimiter.moc"
