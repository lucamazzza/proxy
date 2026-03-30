#include <QtTest>

#include "messageprocessor.h"
#include "state.h"
#include "model.h"
#include "irecoverymanager.h"

using namespace appcomm;
using namespace appcomm::client;

/*!
 * @brief Spy implementation of IRecoveryManager for testing.
 */
class RecoveryManagerSpy : public IRecoveryManager {
public:
    bool requestFromCalled = false;
    QString requestedFromMessageId;

    void requestFrom(const QString& messageId) override
    {
        requestFromCalled = true;
        requestedFromMessageId = messageId;
    }
};

/*!
 * @brief Test class for MessageProcessor.
 */
class tst_messageprocessor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void processIncoming_validFirstMessage_returnsMessageAndUpdatesState();
    void processIncoming_invalidMessage_returnsNullopt();
    void processIncoming_wrongChannel_returnsNullopt();
    void processIncoming_emptyActiveChannel_acceptsMessage();
    void processIncoming_duplicateMessage_returnsNullopt();
    void processIncoming_olderMessage_returnsNullopt();
    void processIncoming_messageWithGap_triggersRecoveryAndReturnsNullopt();

    void isDuplicate_noPreviousMessage_returnsFalse();
    void isDuplicate_sameSequence_returnsTrue();
    void isDuplicate_lowerSequence_returnsTrue();
    void isDuplicate_higherSequence_returnsFalse();

    void hasGap_noPreviousMessage_returnsFalse();
    void hasGap_consecutiveSequence_returnsFalse();
    void hasGap_sameSequence_returnsFalse();
    void hasGap_skippedSequence_returnsTrue();

    void reset_setsChannelAndResetsState();

private:
    model::Message makeValidMessage(
        const QString& channelId = "channel-1",
        qint64 sequenceNumber = 0,
        const QString& messageId = "msg-1"
        ) const
    {
        model::Message msg;
        msg.channelId = channelId;
        msg.senderId = "user-1";
        msg.messageId = messageId;
        msg.sequenceNumber = sequenceNumber;
        msg.timestamp = QDateTime::currentDateTimeUtc();
        msg.payload = QJsonObject{
            {"text", "hello"}
        };
        msg.isEcho = false;
        return msg;
    }
};

void tst_messageprocessor::initTestCase() {}
void tst_messageprocessor::cleanupTestCase() {}

void tst_messageprocessor::processIncoming_validFirstMessage_returnsMessageAndUpdatesState()
{
    ClientState state;
    state.activeChannelId = "channel-1";
    state.lastReceivedSequence = -1;
    state.lastReceivedMessageId = "";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    const auto msg = makeValidMessage("channel-1", 0, "msg-1");
    const auto result = processor.processIncoming(msg);

    QVERIFY(result.has_value());
    QCOMPARE(result->channelId, QString("channel-1"));
    QCOMPARE(result->sequenceNumber, static_cast<qint64>(0));
    QCOMPARE(result->messageId, QString("msg-1"));

    QCOMPARE(state.lastReceivedSequence, static_cast<qint64>(0));
    QCOMPARE(state.lastReceivedMessageId, QString("msg-1"));

    QVERIFY(!recoverySpy.requestFromCalled);
}

void tst_messageprocessor::processIncoming_invalidMessage_returnsNullopt()
{
    ClientState state;
    state.activeChannelId = "channel-1";
    state.lastReceivedSequence = -1;
    state.lastReceivedMessageId = "";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    model::Message invalidMessage;

    const auto result = processor.processIncoming(invalidMessage);

    QVERIFY(!result.has_value());
    QCOMPARE(state.lastReceivedSequence, static_cast<qint64>(-1));
    QCOMPARE(state.lastReceivedMessageId, QString(""));
    QVERIFY(!recoverySpy.requestFromCalled);
}

void tst_messageprocessor::processIncoming_wrongChannel_returnsNullopt()
{
    ClientState state;
    state.activeChannelId = "channel-1";
    state.lastReceivedSequence = -1;
    state.lastReceivedMessageId = "";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    const auto msg = makeValidMessage("channel-2", 0, "msg-1");
    const auto result = processor.processIncoming(msg);

    QVERIFY(!result.has_value());
    QCOMPARE(state.lastReceivedSequence, static_cast<qint64>(-1));
    QCOMPARE(state.lastReceivedMessageId, QString(""));
    QVERIFY(!recoverySpy.requestFromCalled);
}

void tst_messageprocessor::processIncoming_emptyActiveChannel_acceptsMessage()
{
    ClientState state;
    state.activeChannelId = "";
    state.lastReceivedSequence = -1;
    state.lastReceivedMessageId = "";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    const auto msg = makeValidMessage("another-channel", 0, "msg-1");
    const auto result = processor.processIncoming(msg);

    QVERIFY(result.has_value());
    QCOMPARE(result->channelId, QString("another-channel"));
    QCOMPARE(state.lastReceivedSequence, static_cast<qint64>(0));
    QCOMPARE(state.lastReceivedMessageId, QString("msg-1"));
    QVERIFY(!recoverySpy.requestFromCalled);
}

void tst_messageprocessor::processIncoming_duplicateMessage_returnsNullopt()
{
    ClientState state;
    state.activeChannelId = "channel-1";
    state.lastReceivedSequence = 5;
    state.lastReceivedMessageId = "msg-5";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    const auto msg = makeValidMessage("channel-1", 5, "msg-dup");
    const auto result = processor.processIncoming(msg);

    QVERIFY(!result.has_value());
    QCOMPARE(state.lastReceivedSequence, static_cast<qint64>(5));
    QCOMPARE(state.lastReceivedMessageId, QString("msg-5"));
    QVERIFY(!recoverySpy.requestFromCalled);
}

void tst_messageprocessor::processIncoming_olderMessage_returnsNullopt()
{
    ClientState state;
    state.activeChannelId = "channel-1";
    state.lastReceivedSequence = 5;
    state.lastReceivedMessageId = "msg-5";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    const auto msg = makeValidMessage("channel-1", 3, "msg-old");
    const auto result = processor.processIncoming(msg);

    QVERIFY(!result.has_value());
    QCOMPARE(state.lastReceivedSequence, static_cast<qint64>(5));
    QCOMPARE(state.lastReceivedMessageId, QString("msg-5"));
    QVERIFY(!recoverySpy.requestFromCalled);
}

void tst_messageprocessor::processIncoming_messageWithGap_triggersRecoveryAndReturnsNullopt()
{
    ClientState state;
    state.activeChannelId = "channel-1";
    state.lastReceivedSequence = 5;
    state.lastReceivedMessageId = "msg-5";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    const auto msg = makeValidMessage("channel-1", 8, "msg-8");
    const auto result = processor.processIncoming(msg);

    QVERIFY(!result.has_value());
    QCOMPARE(state.lastReceivedSequence, static_cast<qint64>(5));
    QCOMPARE(state.lastReceivedMessageId, QString("msg-5"));

    QVERIFY(recoverySpy.requestFromCalled);
    QCOMPARE(recoverySpy.requestedFromMessageId, QString("msg-5"));
}

void tst_messageprocessor::isDuplicate_noPreviousMessage_returnsFalse()
{
    ClientState state;
    state.lastReceivedSequence = -1;
    state.lastReceivedMessageId = "";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    const auto msg = makeValidMessage("channel-1", 0, "msg-1");
    QVERIFY(!processor.isDuplicate(msg));
}

void tst_messageprocessor::isDuplicate_sameSequence_returnsTrue()
{
    ClientState state;
    state.lastReceivedSequence = 4;
    state.lastReceivedMessageId = "msg-4";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    const auto msg = makeValidMessage("channel-1", 4, "msg-1");
    QVERIFY(processor.isDuplicate(msg));
}

void tst_messageprocessor::isDuplicate_lowerSequence_returnsTrue()
{
    ClientState state;
    state.lastReceivedSequence = 4;
    state.lastReceivedMessageId = "msg-4";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    const auto msg = makeValidMessage("channel-1", 2, "msg-1");
    QVERIFY(processor.isDuplicate(msg));
}

void tst_messageprocessor::isDuplicate_higherSequence_returnsFalse()
{
    ClientState state;
    state.lastReceivedSequence = 4;
    state.lastReceivedMessageId = "msg-4";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    const auto msg = makeValidMessage("channel-1", 5, "msg-1");
    QVERIFY(!processor.isDuplicate(msg));
}

void tst_messageprocessor::hasGap_noPreviousMessage_returnsFalse()
{
    ClientState state;
    state.lastReceivedSequence = -1;
    state.lastReceivedMessageId = "";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    const auto msg = makeValidMessage("channel-1", 10, "msg-10");
    QVERIFY(!processor.hasGap(msg));
}

void tst_messageprocessor::hasGap_consecutiveSequence_returnsFalse()
{
    ClientState state;
    state.lastReceivedSequence = 7;
    state.lastReceivedMessageId = "msg-7";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    const auto msg = makeValidMessage("channel-1", 8, "msg-8");
    QVERIFY(!processor.hasGap(msg));
}

void tst_messageprocessor::hasGap_sameSequence_returnsFalse()
{
    ClientState state;
    state.lastReceivedSequence = 7;
    state.lastReceivedMessageId = "msg-7";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    const auto msg = makeValidMessage("channel-1", 7, "msg-7-again");
    QVERIFY(!processor.hasGap(msg));
}

void tst_messageprocessor::hasGap_skippedSequence_returnsTrue()
{
    ClientState state;
    state.lastReceivedSequence = 7;
    state.lastReceivedMessageId = "msg-7";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    const auto msg = makeValidMessage("channel-1", 10, "msg-10");
    QVERIFY(processor.hasGap(msg));
}

void tst_messageprocessor::reset_setsChannelAndResetsState()
{
    ClientState state;
    state.activeChannelId = "old-channel";
    state.lastReceivedSequence = 10;
    state.lastReceivedMessageId = "msg-10";

    RecoveryManagerSpy recoverySpy;
    MessageProcessor processor(&state, &recoverySpy);

    processor.reset("new-channel");

    QCOMPARE(state.activeChannelId, QString("new-channel"));
    QCOMPARE(state.lastReceivedSequence, static_cast<qint64>(-1));
    QCOMPARE(state.lastReceivedMessageId, QString(""));
}

QTEST_APPLESS_MAIN(tst_messageprocessor)
#include "tst_messageprocessor.moc"
