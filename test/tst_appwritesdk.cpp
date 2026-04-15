#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QSignalSpy>
#include <QTimer>
#include <QEventLoop>
#include <QTest>
#include <QUuid>
#include <functional>

#include "appwritesdk.h"

class tst_appwritesdk : public QObject
{
    Q_OBJECT

private:
    struct RequestResult {
        bool ok = false;
        QJsonObject data;
        int code = 0;
        QString message;
    };

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testIncrementalProvisioningFlow();

private:
    RequestResult waitForServerRequest(const std::function<void()> &request, int timeoutMs = 30000);
    RequestResult waitForServerRequestWithRetry(const std::function<void()> &request,
                                                int attempts,
                                                int retryDelayMs,
                                                int timeoutMs = 30000);
    QString formatFailure(const QString &operation, const RequestResult &result) const;
    void runCleanupRequest(const std::function<void()> &request);

    QNetworkAccessManager *m_network = nullptr;
    appwritesdk::Server *m_server = nullptr;
    appwritesdk::ConnectionConfig m_config;
    QString m_suffix;
    QString m_userId;
    QString m_documentId;
    bool m_databaseCreated = false;
    bool m_collectionCreated = false;
    bool m_userCreated = false;
};

void tst_appwritesdk::initTestCase()
{
    const QString endpoint = qEnvironmentVariable("APPWRITE_ENDPOINT").trimmed();
    const QString projectId = qEnvironmentVariable("APPWRITE_PROJECT_ID").trimmed();
    const QString apiKey = qEnvironmentVariable("APPWRITE_API_KEY").trimmed();

    if (endpoint.isEmpty() || projectId.isEmpty() || apiKey.isEmpty()) {
        QSKIP("Set APPWRITE_ENDPOINT, APPWRITE_PROJECT_ID, and APPWRITE_API_KEY to run Appwrite integration tests.");
    }

    m_suffix = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_suffix.remove('-');
    m_suffix = m_suffix.left(12);

    m_config.endpoint = endpoint;
    m_config.projectId = projectId;
    m_config.apiKey = apiKey;
    m_config.dbId = QString("sdkdb_%1").arg(m_suffix);
    m_config.collectionId = QString("sdkcol_%1").arg(m_suffix);

    m_network = new QNetworkAccessManager(this);
    m_server = new appwritesdk::Server(m_network, this);
    QVERIFY2(m_network != nullptr, "Failed to allocate QNetworkAccessManager.");
    QVERIFY2(m_server != nullptr, "Failed to allocate appwritesdk::Server.");
}

void tst_appwritesdk::cleanupTestCase()
{
    if (!m_server) {
        return;
    }

    if (!m_documentId.isEmpty()) {
        runCleanupRequest([this]() { m_server->deleteDocument(m_config, m_documentId); });
    }

    if (m_collectionCreated) {
        runCleanupRequest([this]() { m_server->deleteCollection(m_config); });
    }

    if (m_databaseCreated) {
        runCleanupRequest([this]() { m_server->deleteDatabase(m_config); });
    }

    if (m_userCreated && !m_userId.isEmpty()) {
        runCleanupRequest([this]() { m_server->deleteUser(m_config, m_userId); });
    }
}

void tst_appwritesdk::testIncrementalProvisioningFlow()
{
    QVERIFY2(m_network != nullptr, "Network manager is null.");
    QVERIFY2(m_server != nullptr, "Server SDK is null.");

    const QString userEmail = QString("sdk.%1@example.com").arg(m_suffix);
    const QString userPassword = QString("ProxySdk%1!").arg(m_suffix.left(6));

    const RequestResult userResult = waitForServerRequest([&]() {
        m_server->createUser(m_config, userEmail, userPassword, "SDK integration user");
    });
    QVERIFY2(userResult.ok, qPrintable(formatFailure("createUser", userResult)));
    m_userId = userResult.data.value("$id").toString();
    QVERIFY2(!m_userId.isEmpty(), "createUser response did not include user id.");
    m_userCreated = true;

    QJsonArray userQueries;
    userQueries.append(QString("search(\"%1\")").arg(userEmail));
    const RequestResult listUsersResult = waitForServerRequest([&]() {
        m_server->listUsers(m_config, userQueries);
    });
    QVERIFY2(listUsersResult.ok, qPrintable(formatFailure("listUsers", listUsersResult)));

    const QJsonArray users = listUsersResult.data.value("users").toArray();
    bool userFound = false;
    for (const QJsonValue &value : users) {
        if (value.toObject().value("$id").toString() == m_userId) {
            userFound = true;
            break;
        }
    }
    QVERIFY2(userFound, "The created user was not found in listUsers response.");

    const RequestResult dbResult = waitForServerRequest([&]() {
        m_server->createDatabase(m_config, QString("SDK Integration %1").arg(m_suffix));
    });
    QVERIFY2(dbResult.ok, qPrintable(formatFailure("createDatabase", dbResult)));
    m_databaseCreated = true;

    const RequestResult collectionResult = waitForServerRequest([&]() {
        m_server->createCollection(m_config, QString("SDK Collection %1").arg(m_suffix));
    });
    QVERIFY2(collectionResult.ok, qPrintable(formatFailure("createCollection", collectionResult)));
    m_collectionCreated = true;

    QJsonObject stringOptions;
    stringOptions["size"] = 255;
    const RequestResult stringAttributeResult = waitForServerRequest([&]() {
        m_server->createAttribute(m_config, "string", "message", true, stringOptions);
    });
    QVERIFY2(stringAttributeResult.ok, qPrintable(formatFailure("createAttribute(message)", stringAttributeResult)));

    QJsonObject integerOptions;
    integerOptions["min"] = 0;
    integerOptions["max"] = 1000000;
    const RequestResult integerAttributeResult = waitForServerRequest([&]() {
        m_server->createAttribute(m_config, "integer", "sequence", true, integerOptions);
    });
    QVERIFY2(integerAttributeResult.ok, qPrintable(formatFailure("createAttribute(sequence)", integerAttributeResult)));

    const QJsonObject documentData {
        {"message", "hello from integration test"},
        {"sequence", 1}
    };
    const RequestResult documentResult = waitForServerRequestWithRetry(
        [&]() { m_server->createDocument(m_config, documentData); }, 20, 1000, 15000);
    QVERIFY2(documentResult.ok, qPrintable(formatFailure("createDocument", documentResult)));
    m_documentId = documentResult.data.value("$id").toString();
    QVERIFY2(!m_documentId.isEmpty(), "createDocument response did not include document id.");

    const RequestResult listDocumentsResult = waitForServerRequest([&]() {
        m_server->listDocuments(m_config);
    });
    QVERIFY2(listDocumentsResult.ok, qPrintable(formatFailure("listDocuments", listDocumentsResult)));

    const QJsonArray documents = listDocumentsResult.data.value("documents").toArray();
    bool documentFound = false;
    for (const QJsonValue &value : documents) {
        if (value.toObject().value("$id").toString() == m_documentId) {
            documentFound = true;
            break;
        }
    }
    QVERIFY2(documentFound, "The created document was not found in listDocuments response.");

    const QString indexKey = QString("idx_msg_seq_%1").arg(m_suffix.left(6));
    const RequestResult indexResult = waitForServerRequest([&]() {
        m_server->createIndex(m_config, indexKey, "key", QStringList {"message", "sequence"});
    });
    QVERIFY2(indexResult.ok, qPrintable(formatFailure("createIndex", indexResult)));
    QVERIFY2(indexResult.data.contains("key") || indexResult.data.contains("$id"),
             "createIndex response did not include index metadata.");
}

tst_appwritesdk::RequestResult tst_appwritesdk::waitForServerRequest(const std::function<void()> &request,
                                                                      int timeoutMs)
{
    if (!m_server) {
        RequestResult result;
        result.message = "Server SDK instance is null.";
        return result;
    }

    RequestResult result;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    QMetaObject::Connection successConn = connect(
        m_server, &appwritesdk::Server::requestSuccess, &loop, [&](const QJsonObject &data) {
            result.ok = true;
            result.data = data;
            loop.quit();
        });
    QMetaObject::Connection errorConn = connect(
        m_server, &appwritesdk::Server::requestError, &loop, [&](int code, const QString &message) {
            result.code = code;
            result.message = message;
            loop.quit();
        });
    connect(&timer, &QTimer::timeout, &loop, [&]() {
        result.message = QString("Timed out after %1 ms").arg(timeoutMs);
        loop.quit();
    });

    request();
    timer.start(timeoutMs);
    loop.exec();

    disconnect(successConn);
    disconnect(errorConn);
    return result;
}

tst_appwritesdk::RequestResult tst_appwritesdk::waitForServerRequestWithRetry(
    const std::function<void()> &request,
    int attempts,
    int retryDelayMs,
    int timeoutMs)
{
    RequestResult lastResult;
    for (int attempt = 0; attempt < attempts; ++attempt) {
        lastResult = waitForServerRequest(request, timeoutMs);
        if (lastResult.ok) {
            return lastResult;
        }
        if (attempt + 1 < attempts) {
            QTest::qWait(retryDelayMs);
        }
    }
    return lastResult;
}

QString tst_appwritesdk::formatFailure(const QString &operation, const RequestResult &result) const
{
    return QString("%1 failed (%2): %3").arg(operation).arg(result.code).arg(result.message);
}

void tst_appwritesdk::runCleanupRequest(const std::function<void()> &request)
{
    const RequestResult result = waitForServerRequest(request, 15000);
    if (!result.ok) {
        qWarning() << "Cleanup request failed:" << result.code << result.message;
    }
}

QTEST_MAIN(tst_appwritesdk)

#include "tst_appwritesdk.moc"
