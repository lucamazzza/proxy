#include "messageprocessor.h"

namespace appcomm {

namespace client {

MessageProcessor::MessageProcessor(ClientState* clientState, IRecoveryManager* recoveryManager)
    : m_clientState(clientState)
    , m_recoveryManager(recoveryManager)
{
}

std::optional<model::Message> MessageProcessor::processIncoming(
    const model::Message& message
    ) {

    if (!message.isValid()) {
        return std::nullopt;
    }

    if (!m_clientState->activeChannelId.isEmpty() &&
        message.channelId != m_clientState->activeChannelId) {
        return std::nullopt;
    }

    if (isDuplicate(message)) {
        return std::nullopt;
    }

    if (hasGap(message)) {
        m_recoveryManager->requestFrom(m_clientState->lastReceivedMessageId);
        return std::nullopt;
    }

    m_clientState->lastReceivedSequence = message.sequenceNumber;
    m_clientState->lastReceivedMessageId = message.messageId;

    return message;
}

bool MessageProcessor::isDuplicate(const model::Message& message) const {
    return message.sequenceNumber <= m_clientState->lastReceivedSequence;
}

bool MessageProcessor::hasGap(const model::Message& message) const {
    if (m_clientState->lastReceivedSequence < 0) {
        return false;
    }

    return message.sequenceNumber > m_clientState->lastReceivedSequence + 1;
}

void MessageProcessor::reset(const QString& channelId) {
    m_clientState->activeChannelId = channelId;
    m_clientState->lastReceivedSequence = -1;
    m_clientState->lastReceivedMessageId.clear();
}

} //namespace client

} //namespace appcomm
