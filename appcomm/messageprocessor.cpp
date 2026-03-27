#include "messageprocessor.h"

namespace appcomm {

namespace client {

MessageProcessor::MessageProcessor(ClientState* clientState)
    : m_clientState(clientState){
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
        //Recovery
    }

    m_clientState->lastReceivedSequence = message.sequenceNumber;

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
}

} //namespace client

} //namespace appcomm
