#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include "model.h"
#include "recoverymanager.h"
#include "state.h"

#include <QString>
#include <optional>

namespace appcomm {
namespace client {

/*!
 * @brief Processes incoming messages and ensures their validity and consistency.
 *
 * The MessageProcessor is responsible for handling messages received from the backend.
 * It performs validation, detects duplicates, and checks for gaps in the message sequence
 * based on the current client state.
 *
 * It does not own the ClientState, but operates on it to maintain consistency between
 * received messages and the client’s internal state.
 */
class MessageProcessor {
public:

    /*!
     * @brief Constructs a MessageProcessor with a reference to the client state.
     *
     * @param clientState Pointer to the ClientState used to track message processing.
     *
     * @note The pointer must not be null and is not owned by this class.
     */
    explicit MessageProcessor(ClientState* clientState, IRecoveryManager* recoveryManager);

    /*!
     * @brief Processes an incoming message.
     *
     * Validates the message, checks for duplicates and gaps, and determines whether
     * the message should be accepted or discarded.
     *
     * @param message The incoming message to process.
     * @return An optional containing the processed message if valid and accepted,
     *         or std::nullopt if the message is invalid, duplicate, or cannot be processed.
     */
    std::optional<model::Message> processIncoming(
        const model::Message& message
        );

    /*!
     * @brief Checks whether there is a gap in the message sequence.
     *
     * A gap occurs when the incoming message does not follow the expected sequence
     * based on the last received message stored in the client state.
     *
     * @param message The incoming message to analyze.
     * @return true if a gap is detected, false otherwise.
     */
    bool hasGap(const model::Message& message) const;

    /*!
     * @brief Checks whether a message is a duplicate.
     *
     * A message is considered a duplicate if it has already been processed,
     * based on the client state (e.g., same messageId as the last received message).
     *
     * @param message The message to check.
     * @return true if the message is a duplicate, false otherwise.
     */
    bool isDuplicate(const model::Message& message) const;

    /*!
     * @brief Resets the message processing state for a given channel.
     *
     * Clears or reinitializes the relevant state associated with the specified channel,
     * allowing message processing to restart from a clean state.
     *
     * @param channelId Identifier of the channel to reset.
     */
    void reset(const QString& channelId);

private:

    /*!
     * @brief Pointer to the client state used for message processing.
     *
     * Stores information such as the last received message and active channel.
     * This pointer is not owned by MessageProcessor.
     */
    ClientState* m_clientState;

    IRecoveryManager *m_recoveryManager;
};

} //namespace client

} //namespace appcomm

#endif // MESSAGEPROCESSOR_H
