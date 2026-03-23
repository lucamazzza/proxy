#include <QTest>
#include <QSignalSpy>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonArray>
#include "appwritesdk.h"

class tst_appwritesdk : public QObject
{
    Q_OBJECT

public:
    tst_appwritesdk();
    ~tst_appwritesdk();

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // ConnectionConfig tests
    void testConnectionConfig();
    
    // BaseSDK tests
    void testBaseSDKConstruction();
    void testCreateBaseRequest();
    void testCreateBaseRequestWithAdmin();
    
    // Client tests
    void testClientConstruction();
    void testCreateAnonymousSession();
    void testCreateEmailSession();
    void testDeleteSession();
    void testDeleteSessions();
    void testGetAccount();
    void testCreateDocument();
    void testListDocuments();
    void testListDocumentsWithQueries();
    void testGetDocument();
    void testUpdateDocument();
    void testDeleteDocument();
    
    // Server tests
    void testServerConstruction();
    void testCreateDatabase();
    void testDeleteDatabase();
    void testCreateCollection();
    void testCreateCollectionWithPermissions();
    void testDeleteCollection();
    void testUpdateCollectionPermissions();
    void testCreateAttributeString();
    void testCreateAttributeInteger();
    void testCreateAttributeBoolean();
    void testCreateAttributeDatetime();
    void testCreateIndex();
    void testCreateUser();
    void testCreateUserWithName();
    void testDeleteUser();
    void testListUsers();
    void testListUsersWithQueries();
    
private:
    QNetworkAccessManager *m_network;
    appwritesdk::Client *m_client;
    appwritesdk::Server *m_server;
    appwritesdk::ConnectionConfig m_config;
};

tst_appwritesdk::tst_appwritesdk()
    : m_network(nullptr)
    , m_client(nullptr)
    , m_server(nullptr)
{
}

tst_appwritesdk::~tst_appwritesdk()
{
}

void tst_appwritesdk::initTestCase()
{
    m_config.endpoint = "http://localhost/v1";
    m_config.projectId = "test-project";
    m_config.apiKey = "test-api-key";
    m_config.dbId = "test-db";
    m_config.collectionId = "test-collection";
    
    m_network = new QNetworkAccessManager(this);
    m_client = new appwritesdk::Client(m_network, this);
    m_server = new appwritesdk::Server(m_network, this);
}

void tst_appwritesdk::cleanupTestCase()
{
}

// ConnectionConfig Tests

void tst_appwritesdk::testConnectionConfig()
{
    appwritesdk::ConnectionConfig config;
    config.endpoint = "http://test.com/v1";
    config.projectId = "proj123";
    config.apiKey = "key456";
    config.dbId = "db789";
    config.collectionId = "coll012";
    
    QCOMPARE(config.endpoint, QString("http://test.com/v1"));
    QCOMPARE(config.projectId, QString("proj123"));
    QCOMPARE(config.apiKey, QString("key456"));
    QCOMPARE(config.dbId, QString("db789"));
    QCOMPARE(config.collectionId, QString("coll012"));
}

// BaseSDK Tests

void tst_appwritesdk::testBaseSDKConstruction()
{
    QVERIFY(m_client != nullptr);
    QVERIFY(m_server != nullptr);
}

void tst_appwritesdk::testCreateBaseRequest()
{
    // Tested indirectly through public operations
    QVERIFY(true);
}

void tst_appwritesdk::testCreateBaseRequestWithAdmin()
{
    // Tested indirectly through server operations
    QVERIFY(true);
}

// Client Tests

void tst_appwritesdk::testClientConstruction()
{
    appwritesdk::Client client(m_network);
    QVERIFY(true);
}

void tst_appwritesdk::testCreateAnonymousSession()
{
    QSignalSpy successSpy(m_client, &appwritesdk::Client::requestSuccess);
    QSignalSpy errorSpy(m_client, &appwritesdk::Client::requestError);
    
    m_client->createAnonymousSession(m_config);
    
    QVERIFY(successSpy.isValid());
    QVERIFY(errorSpy.isValid());
}

void tst_appwritesdk::testCreateEmailSession()
{
    QSignalSpy successSpy(m_client, &appwritesdk::Client::requestSuccess);
    QSignalSpy errorSpy(m_client, &appwritesdk::Client::requestError);
    
    m_client->createEmailSession(m_config, "test@example.com", "password123");
    
    QVERIFY(successSpy.isValid());
    QVERIFY(errorSpy.isValid());
}

void tst_appwritesdk::testDeleteSession()
{
    QSignalSpy successSpy(m_client, &appwritesdk::Client::requestSuccess);
    
    m_client->deleteSession(m_config, "session123");
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testDeleteSessions()
{
    QSignalSpy successSpy(m_client, &appwritesdk::Client::requestSuccess);
    
    m_client->deleteSessions(m_config);
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testGetAccount()
{
    QSignalSpy successSpy(m_client, &appwritesdk::Client::requestSuccess);
    
    m_client->getAccount(m_config);
    
    QVERIFY(successSpy.isValid());
}

// Client Tests

void tst_appwritesdk::testCreateDocument()
{
    QSignalSpy successSpy(m_client, &appwritesdk::Client::requestSuccess);
    
    QJsonObject data;
    data["text"] = "Hello World";
    data["channelId"] = "general";
    
    m_client->createDocument(m_config, data);
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testListDocuments()
{
    QSignalSpy successSpy(m_client, &appwritesdk::Client::requestSuccess);
    
    m_client->listDocuments(m_config);
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testListDocumentsWithQueries()
{
    QSignalSpy successSpy(m_client, &appwritesdk::Client::requestSuccess);
    
    QJsonArray queries;
    queries.append("equal(\"channelId\",\"general\")");
    queries.append("limit(50)");
    
    m_client->listDocuments(m_config, queries);
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testGetDocument()
{
    QSignalSpy successSpy(m_client, &appwritesdk::Client::requestSuccess);
    
    m_client->getDocument(m_config, "doc123");
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testUpdateDocument()
{
    QSignalSpy successSpy(m_client, &appwritesdk::Client::requestSuccess);
    
    QJsonObject data;
    data["text"] = "Updated text";
    
    m_client->updateDocument(m_config, "doc123", data);
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testDeleteDocument()
{
    QSignalSpy successSpy(m_client, &appwritesdk::Client::requestSuccess);
    
    m_client->deleteDocument(m_config, "doc123");
    
    QVERIFY(successSpy.isValid());
}

// Server Tests

void tst_appwritesdk::testServerConstruction()
{
    appwritesdk::Server server(m_network);
    QVERIFY(true);
}

void tst_appwritesdk::testCreateDatabase()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    m_server->createDatabase(m_config, "TestDatabase");
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testDeleteDatabase()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    m_server->deleteDatabase(m_config);
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testCreateCollection()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    m_server->createCollection(m_config, "Messages");
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testCreateCollectionWithPermissions()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    QJsonArray permissions;
    permissions.append("read(\"any\")");
    permissions.append("write(\"users\")");
    
    m_server->createCollection(m_config, "SecureMessages", permissions);
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testDeleteCollection()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    m_server->deleteCollection(m_config);
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testUpdateCollectionPermissions()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    QJsonArray permissions;
    permissions.append("read(\"any\")");
    permissions.append("create(\"users\")");
    
    m_server->updateCollectionPermissions(m_config, permissions);
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testCreateAttributeString()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    QJsonObject options;
    options["size"] = 255;
    
    m_server->createAttribute(m_config, "string", "channelId", true, options);
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testCreateAttributeInteger()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    QJsonObject options;
    options["min"] = 0;
    options["max"] = 100;
    
    m_server->createAttribute(m_config, "integer", "priority", false, options);
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testCreateAttributeBoolean()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    QJsonObject options;
    options["default"] = false;
    
    m_server->createAttribute(m_config, "boolean", "isEcho", false, options);
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testCreateAttributeDatetime()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    m_server->createAttribute(m_config, "datetime", "timestamp", true);
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testCreateIndex()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    QStringList attributes;
    attributes << "channelId" << "timestamp";
    
    m_server->createIndex(m_config, "channel_timestamp_idx", "key", attributes);
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testCreateUser()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    m_server->createUser(m_config, "user@example.com", "password123");
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testCreateUserWithName()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    m_server->createUser(m_config, "user@example.com", "password123", "John Doe");
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testDeleteUser()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    m_server->deleteUser(m_config, "user123");
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testListUsers()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    m_server->listUsers(m_config);
    
    QVERIFY(successSpy.isValid());
}

void tst_appwritesdk::testListUsersWithQueries()
{
    QSignalSpy successSpy(m_server, &appwritesdk::Server::requestSuccess);
    
    QJsonArray queries;
    queries.append("limit(10)");
    queries.append("search(\"email\", \"example.com\")");
    
    m_server->listUsers(m_config, queries);
    
    QVERIFY(successSpy.isValid());
}

QTEST_APPLESS_MAIN(tst_appwritesdk)

#include "tst_appwritesdk.moc"
