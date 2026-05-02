#include <QTest>
#include "model.h"

using namespace appcomm::model;

class tst_models: public QObject
{
    Q_OBJECT

public:
    tst_models();
    ~tst_models();

private slots:
    void initTestCase();
    void cleanupTestCase();

    //User
    void testUserIsValid_valid();
    void testUserIsValid_missingUserId();
    void testUserIsValid_missingEmail();
    void testUserIsValid_trimmedUserId();
    void testUserIsValid_trimmedEmail();
    void testUserIsValid_bothMissing();

    // Topic
    void testChannelIsValid_valid();
    void testChannelIsValid_missingUserId();
    void testChannelIsValid_missingEmail();
    void testChannelIsValid_trimmedUserId();
    void testChannelIsValid_trimmedEmail();
    void testChannelIsValid_bothMissing();

    // Message - isValid
    void testMessageIsValid_valid();
    void testMessageIsValid_missingTopicId();
    void testMessageIsValid_missingSenderId();
    void testMessageIsValid_missingMessageId();
    void testMessageIsValid_invalidTimestamp();
    void testMessageIsValid_trimmedTopicId();
    void testMessageIsValid_trimmedSenderId();
    void testMessageIsValid_trimmedMessageId();

    // Message - toJson / fromJson
    void testMessageToJson();
    void testMessageFromJson_valid();
    void testMessageFromJson_invalidTypes();
    void testMessageFromJson_missingField();
    void testMessageFromJson_invalidTimestampContent();
    void testMessageRoundTrip();

    //Message - SequenceNumber
    void testMessageFromJson_missingSequenceNumber();
    void testMessageFromJson_invalidSequenceNumberType();

    //SessionInfo
    // SessionInfo
    void testSessionInfoIsExpired_true();
    void testSessionInfoIsExpired_falseFuture();
    void testSessionInfoIsExpired_falseInvalid();
    void testSessionInfoFromJson_valid();
    void testSessionInfoFromJson_invalidTypes();
    void testSessionInfoFromJson_missingField();
    void testSessionInfoFromJson_invalidDateContent();

    // TopicMember
    void testTopicMemberToJson();
    void testTopicMemberFromJson_valid();
    void testTopicMemberFromJson_invalidTypes();
    void testTopicMemberFromJson_missingField();
    void testTopicMemberFromJson_invalidDateContent();
    void testTopicMemberRoundTrip();

    // AppCommConfig
    void testAppCommConfigIsValid_valid();
    void testAppCommConfigIsValid_missingEndpoint();
    void testAppCommConfigIsValid_missingProjectId();
    void testAppCommConfigIsValid_missingApiKey();
    void testAppCommConfigIsValid_missingDatabaseId();
    void testAppCommConfigIsValid_missingMessagesCollectionId();
    void testAppCommConfigIsValid_missingMembersCollectionId();
    void testAppCommConfigIsValid_missingIncomingMessagesCollectionId();
    void testAppCommConfigIsValid_trimmedEndpoint();
    void testAppCommConfigIsValid_trimmedApiKey();
};

tst_models::tst_models() {}
tst_models::~tst_models() {}
void tst_models::initTestCase() {}
void tst_models::cleanupTestCase() {}

void tst_models::testUserIsValid_valid()
{
    User user;
    user.userId = "u1";
    user.email = "test@example.com";

    QVERIFY(user.isValid());
}

void tst_models::testUserIsValid_missingUserId()
{
    User user;
    user.userId = "";
    user.email = "test@example.com";

    QVERIFY(!user.isValid());
}

void tst_models::testUserIsValid_missingEmail()
{
    User user;
    user.userId = "u1";
    user.email = "";

    QVERIFY(!user.isValid());
}

void tst_models::testUserIsValid_trimmedUserId()
{
    User user;
    user.userId = "   ";
    user.email = "test@example.com";

    QVERIFY(!user.isValid());
}

void tst_models::testUserIsValid_trimmedEmail()
{
    User user;
    user.userId = "u1";
    user.email = "   ";

    QVERIFY(!user.isValid());
}

void tst_models::testUserIsValid_bothMissing()
{
    User user;
    user.userId = "";
    user.email = "";

    QVERIFY(!user.isValid());
}

void tst_models::testChannelIsValid_valid()
{
    Topic topic;
    topic.topicId = "c1";
    topic.name = "channelname";

    QVERIFY(topic.isValid());
}

void tst_models::testChannelIsValid_missingUserId()
{
    Topic topic;
    topic.topicId = "";
    topic.name = "channelname";

    QVERIFY(!topic.isValid());
}

void tst_models::testChannelIsValid_missingEmail()
{
    Topic topic;
    topic.topicId = "c1";
    topic.name = "";

    QVERIFY(!topic.isValid());
}

void tst_models::testChannelIsValid_trimmedUserId()
{
    Topic topic;
    topic.topicId = "   ";
    topic.name = "channelname";

    QVERIFY(!topic.isValid());
}

void tst_models::testChannelIsValid_trimmedEmail()
{
    Topic topic;
    topic.topicId = "c1";
    topic.name = "   ";

    QVERIFY(!topic.isValid());
}

void tst_models::testChannelIsValid_bothMissing()
{
    Topic topic;
    topic.topicId = "";
    topic.name = "";

    QVERIFY(!topic.isValid());
}

void tst_models::testMessageIsValid_valid()
{
    Message msg;
    msg.topicId = "c1";
    msg.senderId = "u1";
    msg.messageId = "m1";
    msg.timestamp = QDateTime::currentDateTimeUtc();
    msg.payload = QJsonObject{{"text", "hello"}};
    msg.isEcho = false;

    QVERIFY(msg.isValid());
}

void tst_models::testMessageIsValid_missingTopicId()
{
    Message msg;
    msg.topicId = "";
    msg.senderId = "u1";
    msg.messageId = "m1";
    msg.timestamp = QDateTime::currentDateTimeUtc();

    QVERIFY(!msg.isValid());
}

void tst_models::testMessageIsValid_missingSenderId()
{
    Message msg;
    msg.topicId = "c1";
    msg.senderId = "";
    msg.messageId = "m1";
    msg.timestamp = QDateTime::currentDateTimeUtc();

    QVERIFY(!msg.isValid());
}

void tst_models::testMessageIsValid_missingMessageId()
{
    Message msg;
    msg.topicId = "c1";
    msg.senderId = "u1";
    msg.messageId = "";
    msg.timestamp = QDateTime::currentDateTimeUtc();

    QVERIFY(!msg.isValid());
}

void tst_models::testMessageIsValid_invalidTimestamp()
{
    Message msg;
    msg.topicId = "c1";
    msg.senderId = "u1";
    msg.messageId = "m1";
    msg.timestamp = QDateTime(); // invalido

    QVERIFY(!msg.isValid());
}

void tst_models::testMessageIsValid_trimmedTopicId()
{
    Message msg;
    msg.topicId = "   ";
    msg.senderId = "u1";
    msg.messageId = "m1";
    msg.timestamp = QDateTime::currentDateTimeUtc();

    QVERIFY(!msg.isValid());
}

void tst_models::testMessageIsValid_trimmedSenderId()
{
    Message msg;
    msg.topicId = "c1";
    msg.senderId = "   ";
    msg.messageId = "m1";
    msg.timestamp = QDateTime::currentDateTimeUtc();

    QVERIFY(!msg.isValid());
}

void tst_models::testMessageIsValid_trimmedMessageId()
{
    Message msg;
    msg.topicId = "c1";
    msg.senderId = "u1";
    msg.messageId = "   ";
    msg.timestamp = QDateTime::currentDateTimeUtc();

    QVERIFY(!msg.isValid());
}

void tst_models::testMessageToJson()
{
    Message msg;
    msg.topicId = "c1";
    msg.senderId = "u1";
    msg.messageId = "m1";
    msg.sequenceNumber = 42;
    msg.timestamp = QDateTime::fromString("2026-03-20T12:00:00Z", Qt::ISODate);
    msg.payload = QJsonObject{
        {"text", "hello"},
        {"value", 42}
    };
    msg.isEcho = true;

    QJsonObject obj = msg.toJson();

    QCOMPARE(obj.value("topicId").toString(), QString("c1"));
    QCOMPARE(obj.value("senderId").toString(), QString("u1"));
    QCOMPARE(obj.value("messageId").toString(), QString("m1"));
    QCOMPARE(obj.value("sequenceNumber").toInt(), 42);
    QCOMPARE(obj.value("payload").toString(), QString("{\"text\":\"hello\",\"value\":42}"));
    QCOMPARE(obj.value("isEcho").toBool(), true);
}

void tst_models::testMessageFromJson_valid()
{
    QJsonObject obj{
        {"topicId", "c1"},
        {"senderId", "u1"},
        {"messageId", "m1"},
        {"sequenceNumber", 42},
        {"timestamp", "2026-03-20T12:00:00Z"},
        {"payload", QJsonObject{
                        {"text", "hello"},
                        {"value", 42}
                    }},
        {"isEcho", false}
    };

    Message msg = Message::fromJson(obj);

    QCOMPARE(msg.topicId, QString("c1"));
    QCOMPARE(msg.senderId, QString("u1"));
    QCOMPARE(msg.messageId, QString("m1"));
    QCOMPARE(msg.sequenceNumber, 42LL);
    QCOMPARE(msg.timestamp, QDateTime::fromString("2026-03-20T12:00:00Z", Qt::ISODate));
    QCOMPARE(msg.payload, QJsonObject({
                              {"text", "hello"},
                              {"value", 42}
                          }));
    QCOMPARE(msg.isEcho, false);
}

void tst_models::testMessageFromJson_invalidTypes()
{
    QJsonObject obj{
        {"topicId", 123}, // dovrebbe essere string
        {"senderId", "u1"},
        {"messageId", "m1"},
        {"sequenceNumber", "wrong-type"},
        {"timestamp", "2026-03-20T12:00:00Z"},
        {"payload", QJsonObject{{"text", "hello"}}},
        {"isEcho", false}
    };

    Message msg = Message::fromJson(obj);

    QVERIFY(msg.topicId.isEmpty());
    QVERIFY(msg.senderId.isEmpty());
    QVERIFY(msg.messageId.isEmpty());
    QCOMPARE(msg.sequenceNumber, -1LL);
    QVERIFY(!msg.timestamp.isValid());
    QVERIFY(msg.payload.isEmpty());
    QCOMPARE(msg.isEcho, false);
}

void tst_models::testMessageFromJson_missingField()
{
    QJsonObject obj{
        // manca topicId
        {"senderId", "u1"},
        {"messageId", "m1"},
        {"sequenceNumber", 42},
        {"timestamp", "2026-03-20T12:00:00Z"},
        {"payload", QJsonObject{{"text", "hello"}}},
        {"isEcho", false}
    };

    Message msg = Message::fromJson(obj);

    QVERIFY(msg.topicId.isEmpty());
    QVERIFY(msg.senderId.isEmpty());
    QVERIFY(msg.messageId.isEmpty());
    QCOMPARE(msg.sequenceNumber, -1LL);
    QVERIFY(!msg.timestamp.isValid());
    QVERIFY(msg.payload.isEmpty());
    QCOMPARE(msg.isEcho, false);
}

void tst_models::testMessageFromJson_invalidTimestampContent()
{
    QJsonObject obj{
        {"topicId", "c1"},
        {"senderId", "u1"},
        {"messageId", "m1"},
        {"sequenceNumber", 42},
        {"timestamp", "not-a-date"}, // string valida come tipo, ma contenuto non parsabile
        {"payload", QJsonObject{{"text", "hello"}}},
        {"isEcho", true}
    };

    Message msg = Message::fromJson(obj);

    QCOMPARE(msg.topicId, QString("c1"));
    QCOMPARE(msg.senderId, QString("u1"));
    QCOMPARE(msg.messageId, QString("m1"));
    QCOMPARE(msg.sequenceNumber, 42LL);
    QVERIFY(!msg.timestamp.isValid()); // fromJson NON rifiuta il contenuto, solo il tipo
    QCOMPARE(msg.payload, QJsonObject({{"text", "hello"}}));
    QCOMPARE(msg.isEcho, true);

    QVERIFY(!msg.isValid()); // qui invece fallisce per timestamp invalido
}

void tst_models::testMessageRoundTrip()
{
    Message original;
    original.topicId = "c1";
    original.senderId = "u1";
    original.messageId = "m1";
    original.sequenceNumber = 42;
    original.timestamp = QDateTime::fromString("2026-03-20T12:00:00Z", Qt::ISODate);
    original.payload = QJsonObject{
        {"text", "hello"},
        {"value", 42},
        {"flag", true}
    };
    original.isEcho = true;

    QJsonObject json = original.toJson();
    Message copy = Message::fromJson(json);

    QCOMPARE(copy.topicId, original.topicId);
    QCOMPARE(copy.senderId, original.senderId);
    QCOMPARE(copy.messageId, original.messageId);
    QCOMPARE(copy.timestamp, original.timestamp);
    QCOMPARE(copy.payload, original.payload);
    QCOMPARE(copy.isEcho, original.isEcho);
}

void tst_models::testMessageFromJson_missingSequenceNumber()
{
    QJsonObject obj{
        {"topicId", "c1"},
        {"senderId", "u1"},
        {"messageId", "m1"},
        {"timestamp", "2026-03-20T12:00:00Z"},
        {"payload", QJsonObject{{"text", "hello"}}},
        {"isEcho", false}
    };

    Message msg = Message::fromJson(obj);

    QCOMPARE(msg.topicId, QString("c1"));
    QCOMPARE(msg.senderId, QString("u1"));
    QCOMPARE(msg.messageId, QString("m1"));
    QCOMPARE(msg.sequenceNumber, -1LL);
    QCOMPARE(msg.timestamp, QDateTime::fromString("2026-03-20T12:00:00Z", Qt::ISODate));
    QCOMPARE(msg.payload, QJsonObject({{"text", "hello"}}));
    QCOMPARE(msg.isEcho, false);
}

void tst_models::testMessageFromJson_invalidSequenceNumberType()
{
    QJsonObject obj{
        {"topicId", "c1"},
        {"senderId", "u1"},
        {"messageId", "m1"},
        {"sequenceNumber", "abc"},
        {"timestamp", "2026-03-20T12:00:00Z"},
        {"payload", QJsonObject{{"text", "hello"}}},
        {"isEcho", false}
    };

    Message msg = Message::fromJson(obj);

    QCOMPARE(msg.topicId, QString("c1"));
    QCOMPARE(msg.senderId, QString("u1"));
    QCOMPARE(msg.messageId, QString("m1"));
    QCOMPARE(msg.sequenceNumber, -1LL);
    QCOMPARE(msg.timestamp, QDateTime::fromString("2026-03-20T12:00:00Z", Qt::ISODate));
    QCOMPARE(msg.payload, QJsonObject({{"text", "hello"}}));
    QCOMPARE(msg.isEcho, false);
}

void tst_models::testSessionInfoIsExpired_true()
{
    SessionInfo session;
    session.expiresAt = QDateTime::currentDateTimeUtc().addSecs(-10);

    QVERIFY(session.isExpired());
}

void tst_models::testSessionInfoIsExpired_falseFuture()
{
    SessionInfo session;
    session.expiresAt = QDateTime::currentDateTimeUtc().addSecs(3600);

    QVERIFY(!session.isExpired());
}

void tst_models::testSessionInfoIsExpired_falseInvalid()
{
    SessionInfo session;
    session.expiresAt = QDateTime(); // invalido

    QVERIFY(!session.isExpired());
}

void tst_models::testSessionInfoFromJson_valid()
{
    QJsonObject obj{
        {"userId", "u1"},
        {"sessionId", "s1"},
        {"authType", 1},
        {"createdAt", "2026-03-20T10:00:00Z"},
        {"expiresAt", "2026-03-20T12:00:00Z"}
    };

    SessionInfo session = SessionInfo::fromJson(obj);

    QCOMPARE(session.userId, QString("u1"));
    QCOMPARE(session.sessionId, QString("s1"));
    QCOMPARE(session.authType, AuthType::Email);
    QCOMPARE(session.createdAt, QDateTime::fromString("2026-03-20T10:00:00Z", Qt::ISODate));
    QCOMPARE(session.expiresAt, QDateTime::fromString("2026-03-20T12:00:00Z", Qt::ISODate));
}

void tst_models::testSessionInfoFromJson_invalidTypes()
{
    QJsonObject obj{
        {"userId", "u1"},
        {"sessionId", "s1"},
        {"authType", "Email"}, // sbagliato: deve essere numero
        {"createdAt", "2026-03-20T10:00:00Z"},
        {"expiresAt", "2026-03-20T12:00:00Z"}
    };

    SessionInfo session = SessionInfo::fromJson(obj);

    QVERIFY(session.userId.isEmpty());
    QVERIFY(session.sessionId.isEmpty());
    QVERIFY(!session.createdAt.isValid());
    QVERIFY(!session.expiresAt.isValid());
}

void tst_models::testSessionInfoFromJson_missingField()
{
    QJsonObject obj{
        {"userId", "u1"},
        // manca sessionId
        {"authType", 1},
        {"createdAt", "2026-03-20T10:00:00Z"},
        {"expiresAt", "2026-03-20T12:00:00Z"}
    };

    SessionInfo session = SessionInfo::fromJson(obj);

    QVERIFY(session.userId.isEmpty());
    QVERIFY(session.sessionId.isEmpty());
    QVERIFY(!session.createdAt.isValid());
    QVERIFY(!session.expiresAt.isValid());
}

void tst_models::testSessionInfoFromJson_invalidDateContent()
{
    QJsonObject obj{
        {"userId", "u1"},
        {"sessionId", "s1"},
        {"authType", 0},
        {"createdAt", "not-a-date"},
        {"expiresAt", "still-not-a-date"}
    };

    SessionInfo session = SessionInfo::fromJson(obj);

    QCOMPARE(session.userId, QString("u1"));
    QCOMPARE(session.sessionId, QString("s1"));
    QCOMPARE(session.authType, AuthType::Guest);
    QVERIFY(!session.createdAt.isValid());
    QVERIFY(!session.expiresAt.isValid());
}

void tst_models::testTopicMemberToJson()
{
    TopicMember member;
    member.userId = "u1";
    member.topicId = "c1";
    member.displayName = "Manu";
    member.joinedAt = QDateTime::fromString("2026-03-20T10:00:00Z", Qt::ISODate);
    member.lastSeenAt = QDateTime::fromString("2026-03-20T11:00:00Z", Qt::ISODate);
    member.isActive = true;

    QJsonObject obj = member.toJson();

    QCOMPARE(obj.value("userId").toString(), QString("u1"));
    QCOMPARE(obj.value("topicId").toString(), QString("c1"));
    QCOMPARE(obj.value("displayName").toString(), QString("Manu"));
    QCOMPARE(obj.value("joinedAt").toString(), QString("2026-03-20T10:00:00Z"));
    QCOMPARE(obj.value("lastSeenAt").toString(), QString("2026-03-20T11:00:00Z"));
    QCOMPARE(obj.value("isActive").toBool(), true);
}

void tst_models::testTopicMemberFromJson_valid()
{
    QJsonObject obj{
        {"userId", "u1"},
        {"topicId", "c1"},
        {"displayName", "Manu"},
        {"joinedAt", "2026-03-20T10:00:00Z"},
        {"lastSeenAt", "2026-03-20T11:00:00Z"},
        {"isActive", true}
    };

    TopicMember member = TopicMember::fromJson(obj);

    QCOMPARE(member.userId, QString("u1"));
    QCOMPARE(member.topicId, QString("c1"));
    QCOMPARE(member.displayName, QString("Manu"));
    QCOMPARE(member.joinedAt, QDateTime::fromString("2026-03-20T10:00:00Z", Qt::ISODate));
    QCOMPARE(member.lastSeenAt, QDateTime::fromString("2026-03-20T11:00:00Z", Qt::ISODate));
    QCOMPARE(member.isActive, true);
}

void tst_models::testTopicMemberFromJson_invalidTypes()
{
    QJsonObject obj{
        {"userId", "u1"},
        {"topicId", "c1"},
        {"displayName", "Manu"},
        {"joinedAt", "2026-03-20T10:00:00Z"},
        {"lastSeenAt", "2026-03-20T11:00:00Z"},
        {"isActive", "true"} // sbagliato: deve essere bool
    };

    TopicMember member = TopicMember::fromJson(obj);

    QVERIFY(member.userId.isEmpty());
    QVERIFY(member.topicId.isEmpty());
    QVERIFY(member.displayName.isEmpty());
    QVERIFY(!member.joinedAt.isValid());
    QVERIFY(!member.lastSeenAt.isValid());
    QCOMPARE(member.isActive, false);
}

void tst_models::testTopicMemberFromJson_missingField()
{
    QJsonObject obj{
        {"userId", "u1"},
        {"topicId", "c1"},
        // manca displayName
        {"joinedAt", "2026-03-20T10:00:00Z"},
        {"lastSeenAt", "2026-03-20T11:00:00Z"},
        {"isActive", true}
    };

    TopicMember member = TopicMember::fromJson(obj);

    QVERIFY(member.userId.isEmpty());
    QVERIFY(member.topicId.isEmpty());
    QVERIFY(member.displayName.isEmpty());
    QVERIFY(!member.joinedAt.isValid());
    QVERIFY(!member.lastSeenAt.isValid());
    QCOMPARE(member.isActive, false);
}

void tst_models::testTopicMemberFromJson_invalidDateContent()
{
    QJsonObject obj{
        {"userId", "u1"},
        {"topicId", "c1"},
        {"displayName", "Manu"},
        {"joinedAt", "not-a-date"},
        {"lastSeenAt", "still-not-a-date"},
        {"isActive", true}
    };

    TopicMember member = TopicMember::fromJson(obj);

    QCOMPARE(member.userId, QString("u1"));
    QCOMPARE(member.topicId, QString("c1"));
    QCOMPARE(member.displayName, QString("Manu"));
    QVERIFY(!member.joinedAt.isValid());
    QVERIFY(!member.lastSeenAt.isValid());
    QCOMPARE(member.isActive, true);
}

void tst_models::testTopicMemberRoundTrip()
{
    TopicMember original;
    original.userId = "u1";
    original.topicId = "c1";
    original.displayName = "Manu";
    original.joinedAt = QDateTime::fromString("2026-03-20T10:00:00Z", Qt::ISODate);
    original.lastSeenAt = QDateTime::fromString("2026-03-20T11:00:00Z", Qt::ISODate);
    original.isActive = true;

    QJsonObject json = original.toJson();
    TopicMember copy = TopicMember::fromJson(json);

    QCOMPARE(copy.userId, original.userId);
    QCOMPARE(copy.topicId, original.topicId);
    QCOMPARE(copy.displayName, original.displayName);
    QCOMPARE(copy.joinedAt, original.joinedAt);
    QCOMPARE(copy.lastSeenAt, original.lastSeenAt);
    QCOMPARE(copy.isActive, original.isActive);
}

void tst_models::testAppCommConfigIsValid_valid()
{
    AppCommConfig config;
    config.endpoint = "https://example.com/v1";
    config.projectId = "project123";
    config.apiKey = "apikey";
    config.databaseId = "db1";
    config.messagesCollectionId = "messages";
    config.incomingMessagesCollectionId = "pendingmessages";
    config.membersCollectionId = "members";

    QVERIFY(config.isValid());
}

void tst_models::testAppCommConfigIsValid_missingEndpoint()
{
    AppCommConfig config;
    config.endpoint = "";
    config.projectId = "project123";
    config.apiKey = "apikey";
    config.databaseId = "db1";
    config.messagesCollectionId = "messages";
    config.membersCollectionId = "members";

    QVERIFY(!config.isValid());
}

void tst_models::testAppCommConfigIsValid_missingProjectId()
{
    AppCommConfig config;
    config.endpoint = "https://example.com/v1";
    config.projectId = "";
    config.apiKey = "apikey";
    config.databaseId = "db1";
    config.messagesCollectionId = "messages";
    config.membersCollectionId = "members";

    QVERIFY(!config.isValid());
}

void tst_models::testAppCommConfigIsValid_missingApiKey()
{
    AppCommConfig config;
    config.endpoint = "https://example.com/v1";
    config.projectId = "project123";
    config.apiKey = "";
    config.databaseId = "db1";
    config.messagesCollectionId = "messages";
    config.membersCollectionId = "members";

    QVERIFY(!config.isValid());
}

void tst_models::testAppCommConfigIsValid_missingDatabaseId()
{
    AppCommConfig config;
    config.endpoint = "https://example.com/v1";
    config.projectId = "project123";
    config.apiKey = "apikey";
    config.databaseId = "";
    config.messagesCollectionId = "messages";
    config.membersCollectionId = "members";

    QVERIFY(!config.isValid());
}

void tst_models::testAppCommConfigIsValid_missingMessagesCollectionId()
{
    AppCommConfig config;
    config.endpoint = "https://example.com/v1";
    config.projectId = "project123";
    config.apiKey = "apikey";
    config.databaseId = "db1";
    config.messagesCollectionId = "";
    config.membersCollectionId = "members";

    QVERIFY(!config.isValid());
}

void tst_models::testAppCommConfigIsValid_missingMembersCollectionId()
{
    AppCommConfig config;
    config.endpoint = "https://example.com/v1";
    config.projectId = "project123";
    config.apiKey = "apikey";
    config.databaseId = "db1";
    config.messagesCollectionId = "messages";
    config.membersCollectionId = "";

    QVERIFY(!config.isValid());
}

void tst_models::testAppCommConfigIsValid_missingIncomingMessagesCollectionId()
{
    AppCommConfig config;
    config.endpoint = "https://example.com/v1";
    config.projectId = "project123";
    config.apiKey = "apikey";
    config.databaseId = "db1";
    config.messagesCollectionId = "messages";
    config.incomingMessagesCollectionId = "";
    config.membersCollectionId = "members";

    QVERIFY(!config.isValid());
}

void tst_models::testAppCommConfigIsValid_trimmedEndpoint()
{
    AppCommConfig config;
    config.endpoint = "   ";
    config.projectId = "project123";
    config.apiKey = "apikey";
    config.databaseId = "db1";
    config.messagesCollectionId = "messages";
    config.membersCollectionId = "members";

    QVERIFY(!config.isValid());
}

void tst_models::testAppCommConfigIsValid_trimmedApiKey()
{
    AppCommConfig config;
    config.endpoint = "https://example.com/v1";
    config.projectId = "project123";
    config.apiKey = "   ";
    config.databaseId = "db1";
    config.messagesCollectionId = "messages";
    config.membersCollectionId = "members";

    QVERIFY(!config.isValid());
}

QTEST_APPLESS_MAIN(tst_models)
#include "tst_models.moc"
