#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include "model.h"
#include "state.h"

#include <QString>
#include <optional>

class MessageProcessor {
public:
    explicit MessageProcessor(appcomm::client::ClientState* clientState);

    std::optional<appcomm::model::Message> processIncoming(
        const appcomm::model::Message& message
        );
    bool hasGap(const appcomm::model::Message& message) const;
    bool isDuplicate(const appcomm::model::Message& message) const;
    void reset(const QString& channelId);

private:
    appcomm::client::ClientState* m_clientState;
};

#endif // MESSAGEPROCESSOR_H
