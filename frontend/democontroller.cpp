#include "DemoController.h"

#include <QDateTime>

DemoController::DemoController(QObject *parent)
    : QObject(parent)
{
}

QString DemoController::connectionState() const
{
    return m_connectionState;
}

QString DemoController::currentChannel() const
{
    return m_currentChannel;
}

QString DemoController::errorMessage() const
{
    return m_errorMessage;
}

bool DemoController::authenticated() const
{
    return m_authenticated;
}

MessageListModel* DemoController::messagesModel()
{
    return &m_messagesModel;
}

void DemoController::loginAsGuest()
{
    setAuthenticated(true);
    setConnectionState("Authenticated");
    setErrorMessage(QString());
    emit loginSucceeded();
}

void DemoController::loginWithEmail(const QString &email, const QString &password)
{
    if (email.trimmed().isEmpty()) {
        setErrorMessage("Email cannot be empty");
        return;
    }

    if (password.isEmpty()) {
        setErrorMessage("Password cannot be empty");
        return;
    }

    setAuthenticated(true);
    setConnectionState("Authenticated");
    setErrorMessage(QString());
    emit loginSucceeded();
}

void DemoController::joinChannel(const QString &channelId)
{
    const QString trimmed = channelId.trimmed();

    if (!m_authenticated) {
        setErrorMessage("You must authenticate first");
        return;
    }

    if (trimmed.isEmpty()) {
        setErrorMessage("Channel ID cannot be empty");
        return;
    }

    setCurrentChannel(trimmed);
    setConnectionState("Connected");
    setErrorMessage(QString());
    addSystemMessage("Joined channel: " + trimmed);
}

void DemoController::leaveChannel()
{
    if (m_currentChannel.isEmpty()) {
        setErrorMessage("No active channel");
        return;
    }

    addSystemMessage("Left channel: " + m_currentChannel);
    setCurrentChannel(QString());
    setConnectionState("Authenticated");
    setErrorMessage(QString());
}

void DemoController::sendMessage(const QString &text)
{
    const QString trimmed = text.trimmed();

    if (m_currentChannel.isEmpty()) {
        setErrorMessage("Join a channel before sending messages");
        return;
    }

    if (trimmed.isEmpty()) {
        return;
    }

    setErrorMessage(QString());
    addUserMessage("me", trimmed);

    // TODO: qui andra' la chiamata reale a AppcommClient
    // Esempio concettuale:
    // QJsonObject payload;
    // payload["text"] = trimmed;
    // m_client->sendMessage(payload);
}

void DemoController::clearError()
{
    setErrorMessage(QString());
}

void DemoController::setConnectionState(const QString &state)
{
    if (m_connectionState == state) {
        return;
    }

    m_connectionState = state;
    emit connectionStateChanged();
}

void DemoController::setCurrentChannel(const QString &channel)
{
    if (m_currentChannel == channel) {
        return;
    }

    m_currentChannel = channel;
    emit currentChannelChanged();
}

void DemoController::setErrorMessage(const QString &message)
{
    if (m_errorMessage == message) {
        return;
    }

    m_errorMessage = message;
    emit errorMessageChanged();
}

void DemoController::setAuthenticated(bool authenticated)
{
    if (m_authenticated == authenticated) {
        return;
    }

    m_authenticated = authenticated;
    emit authenticatedChanged();
}

void DemoController::addSystemMessage(const QString &text)
{
    m_messagesModel.addMessage({
        "system",
        text,
        QDateTime::currentDateTime().toString("HH:mm:ss"),
        true
    });
}

void DemoController::addUserMessage(const QString &sender, const QString &text)
{
    m_messagesModel.addMessage({
        sender,
        text,
        QDateTime::currentDateTime().toString("HH:mm:ss"),
        false
    });
}