#include <QtTest>

#include "messageprocessor.h"
#include "state.h"
#include "model.h"

using namespace appcomm;
using namespace appcomm::client;

/*!
 * @brief Test class for MessageProcessor.
 *
 * Verifies message validation, duplicate detection, gap detection,
 * channel filtering, and reset behavior.
 */
class TestMessageProcessor : public QObject
{
    Q_OBJECT

private slots:
    void processIncoming_validFirstMessage_returnsMessageAndUpdatesSequence();
    void processIncoming_invalidMessage_returnsNullopt();
    void processIncoming_wrongChannel_returnsNullopt();
    void processIncoming_emptyActiveChannel_acceptsMessage();
    void processIncoming_duplicateMessage_returnsNullopt();
    void processIncoming_olderMessage_returnsNullopt();
    //TODO: implementare recovery dei messaggi
    void processIncoming_messageWithGap_returnsMessageAndUpdatesSequence();

    void isDuplicate_noPreviousMessage_returnsFalse();
    void isDuplicate_sameSequence_returnsTrue();
    void isDuplicate_lowerSequence_returnsTrue();
    void isDuplicate_higherSequence_returnsFalse();

    void hasGap_noPreviousMessage_returnsFalse();
    void hasGap_consecutiveSequence_returnsFalse();
    void hasGap_sameSequence_returnsFalse();
    void hasGap_skippedSequence_returnsTrue();

    void reset_setsChannelAndResetsSequence();

private:
    /*!
     * @brief Creates a valid message for testing.
     *
     * Adjust this helper if your model::Message::isValid()
     * requires additional fields.
     */
    appcomm::model::Message makeValidMessage(
        const QString& channelId = "channel-1",
        qint64 sequenceNumber = 0,
        const QString& messageId = "msg-1"
        ) const
    {
        appcomm::model::Message msg;
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

void TestMessageProcessor::processIncoming_validFirstMessage_returnsMessageAndUpdatesSequence()
{
    ClientState state;
    state.activeChannelId = "channel-1";
    state.lastReceivedSequence = -1;

    MessageProcessor processor(&state);
    const auto msg = makeValidMessage("channel-1", 0, "msg-1");

    const auto result = processor.processIncoming(msg);

    QVERIFY(result.has_value());
    QCOMPARE(result->channelId, QString("channel-1"));
    QCOMPARE(result->sequenceNumber, 0);
    QCOMPARE(state.lastReceivedSequence, 0);
}

void TestMessageProcessor::processIncoming_invalidMessage_returnsNullopt()
{
    ClientState state;
    state.activeChannelId = "channel-1";
    state.lastReceivedSequence = -1;

    MessageProcessor processor(&state);

    appcomm::model::Message invalidMessage;
    // Lasciato volutamente incompleto/non valido

    const auto result = processor.processIncoming(invalidMessage);

    QVERIFY(!result.has_value());
    QCOMPARE(state.lastReceivedSequence, -1);
}

void TestMessageProcessor::processIncoming_wrongChannel_returnsNullopt()
{
    ClientState state;
    state.activeChannelId = "channel-1";
    state.lastReceivedSequence = -1;

    MessageProcessor processor(&state);
    const auto msg = makeValidMessage("channel-2", 0, "msg-1");

    const auto result = processor.processIncoming(msg);

    QVERIFY(!result.has_value());
    QCOMPARE(state.lastReceivedSequence, -1);
}

void TestMessageProcessor::processIncoming_emptyActiveChannel_acceptsMessage()
{
    ClientState state;
    state.activeChannelId = "";
    state.lastReceivedSequence = -1;

    MessageProcessor processor(&state);
    const auto msg = makeValidMessage("another-channel", 0, "msg-1");

    const auto result = processor.processIncoming(msg);

    QVERIFY(result.has_value());
    QCOMPARE(result->channelId, QString("another-channel"));
    QCOMPARE(state.lastReceivedSequence, 0);
}

void TestMessageProcessor::processIncoming_duplicateMessage_returnsNullopt()
{
    ClientState state;
    state.activeChannelId = "channel-1";
    state.lastReceivedSequence = 5;

    MessageProcessor processor(&state);
    const auto msg = makeValidMessage("channel-1", 5, "msg-dup");

    const auto result = processor.processIncoming(msg);

    QVERIFY(!result.has_value());
    QCOMPARE(state.lastReceivedSequence, 5);
}

void TestMessageProcessor::processIncoming_olderMessage_returnsNullopt()
{
    ClientState state;
    state.activeChannelId = "channel-1";
    state.lastReceivedSequence = 5;

    MessageProcessor processor(&state);
    const auto msg = makeValidMessage("channel-1", 3, "msg-old");

    const auto result = processor.processIncoming(msg);

    QVERIFY(!result.has_value());
    QCOMPARE(state.lastReceivedSequence, 5);
}

/*
void TestMessageProcessor::processIncoming_messageWithGap_returnsMessageAndUpdatesSequence()
{
    //TODO
}
*/

void TestMessageProcessor::isDuplicate_noPreviousMessage_returnsFalse()
{
    ClientState state;
    state.lastReceivedSequence = -1;

    MessageProcessor processor(&state);
    const auto msg = makeValidMessage("channel-1", 0, "msg-1");

    QVERIFY(!processor.isDuplicate(msg));
}

void TestMessageProcessor::isDuplicate_sameSequence_returnsTrue()
{
    ClientState state;
    state.lastReceivedSequence = 4;

    MessageProcessor processor(&state);
    const auto msg = makeValidMessage("channel-1", 4, "msg-1");

    QVERIFY(processor.isDuplicate(msg));
}

void TestMessageProcessor::isDuplicate_lowerSequence_returnsTrue()
{
    ClientState state;
    state.lastReceivedSequence = 4;

    MessageProcessor processor(&state);
    const auto msg = makeValidMessage("channel-1", 2, "msg-1");

    QVERIFY(processor.isDuplicate(msg));
}

void TestMessageProcessor::isDuplicate_higherSequence_returnsFalse()
{
    ClientState state;
    state.lastReceivedSequence = 4;

    MessageProcessor processor(&state);
    const auto msg = makeValidMessage("channel-1", 5, "msg-1");

    QVERIFY(!processor.isDuplicate(msg));
}

void TestMessageProcessor::hasGap_noPreviousMessage_returnsFalse()
{
    ClientState state;
    state.lastReceivedSequence = -1;

    MessageProcessor processor(&state);
    const auto msg = makeValidMessage("channel-1", 10, "msg-1");

    QVERIFY(!processor.hasGap(msg));
}

void TestMessageProcessor::hasGap_consecutiveSequence_returnsFalse()
{
    ClientState state;
    state.lastReceivedSequence = 7;

    MessageProcessor processor(&state);
    const auto msg = makeValidMessage("channel-1", 8, "msg-1");

    QVERIFY(!processor.hasGap(msg));
}

void TestMessageProcessor::hasGap_sameSequence_returnsFalse()
{
    ClientState state;
    state.lastReceivedSequence = 7;

    MessageProcessor processor(&state);
    const auto msg = makeValidMessage("channel-1", 7, "msg-1");

    QVERIFY(!processor.hasGap(msg));
}

void TestMessageProcessor::hasGap_skippedSequence_returnsTrue()
{
    ClientState state;
    state.lastReceivedSequence = 7;

    MessageProcessor processor(&state);
    const auto msg = makeValidMessage("channel-1", 10, "msg-1");

    QVERIFY(processor.hasGap(msg));
}

void TestMessageProcessor::reset_setsChannelAndResetsSequence()
{
    ClientState state;
    state.activeChannelId = "old-channel";
    state.lastReceivedSequence = 10; //valore casuale diverso da -1

    MessageProcessor processor(&state);

    processor.reset("new-channel");

    QCOMPARE(state.activeChannelId, QString("new-channel"));
    QCOMPARE(state.lastReceivedSequence, -1);
}

QTEST_APPLESS_MAIN(TestMessageProcessor)
#include "tst_messageprocessor.moc"
