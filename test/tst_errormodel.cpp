#include <QTest>
#include "errormodel.h"

using namespace errormodel;

class tst_errormodel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    //errorCodeToString
    void testErrorCodeToString_none();
    void testErrorCodeToString_authError();
    void testErrorCodeToString_networkError();
    void testErrorCodeToString_parseError();
    void testErrorCodeToString_protocolError();
    void testErrorCodeToString_permissionError();
    void testErrorCodeToString_timeoutError();
    void testErrorCodeToString_rateLimitExceeded();
    void testErrorCodeToString_unknown();

    //AppCommError::hasError
    void testAppCommErrorHasError_false();
    void testAppCommErrorHasError_true();

    //AppCommError::toString
    void testAppCommErrorToString_withHttpStatus();
    void testAppCommErrorToString_noneError();
};

void tst_errormodel::initTestCase() {}
void tst_errormodel::cleanupTestCase() {}

//errorCodeToString

void tst_errormodel::testErrorCodeToString_none()
{
    QCOMPARE(errorCodeToString(ErrorCode::None), QString("None"));
}

void tst_errormodel::testErrorCodeToString_authError()
{
    QCOMPARE(errorCodeToString(ErrorCode::AuthError), QString("AuthError"));
}

void tst_errormodel::testErrorCodeToString_networkError()
{
    QCOMPARE(errorCodeToString(ErrorCode::NetworkError), QString("NetworkError"));
}

void tst_errormodel::testErrorCodeToString_parseError()
{
    QCOMPARE(errorCodeToString(ErrorCode::ParseError), QString("ParseError"));
}

void tst_errormodel::testErrorCodeToString_protocolError()
{
    QCOMPARE(errorCodeToString(ErrorCode::ProtocolError), QString("ProtocolError"));
}

void tst_errormodel::testErrorCodeToString_permissionError()
{
    QCOMPARE(errorCodeToString(ErrorCode::PermissionError), QString("PermissionError"));
}

void tst_errormodel::testErrorCodeToString_timeoutError()
{
    QCOMPARE(errorCodeToString(ErrorCode::TimeoutError), QString("TimeoutError"));
}

void tst_errormodel::testErrorCodeToString_rateLimitExceeded()
{
    QCOMPARE(errorCodeToString(ErrorCode::RateLimitExceeded), QString("RateLimitExceeded"));
}

void tst_errormodel::testErrorCodeToString_unknown()
{
    QCOMPARE(errorCodeToString(ErrorCode::Unknown), QString("Unknown"));
}

//AppCommError::hasError

void tst_errormodel::testAppCommErrorHasError_false()
{
    AppCommError error;
    error.code = ErrorCode::None;
    error.message = "";
    error.httpStatus = 0;

    QVERIFY(!error.hasError());
}

void tst_errormodel::testAppCommErrorHasError_true()
{
    AppCommError error;
    error.code = ErrorCode::NetworkError;
    error.message = "Connection failed";
    error.httpStatus = 503;

    QVERIFY(error.hasError());
}

//AppCommError::toString

void tst_errormodel::testAppCommErrorToString_withHttpStatus()
{
    AppCommError error;
    error.code = ErrorCode::AuthError;
    error.message = "Invalid credentials";
    error.httpStatus = 401;

    QCOMPARE(error.toString(), QString("Error[AuthError] (401): Invalid credentials"));
}

void tst_errormodel::testAppCommErrorToString_noneError()
{
    AppCommError error;
    error.code = ErrorCode::None;
    error.message = "No error";
    error.httpStatus = 0;

    QCOMPARE(error.toString(), QString("Error[None] (0): No error"));
}

QTEST_APPLESS_MAIN(tst_errormodel)
#include "tst_errormodel.moc"
