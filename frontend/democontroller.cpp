#include "democontroller.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>

DemoController::DemoController(const appcomm::model::AppCommConfig &config,
                               QObject *parent)
    : QObject(parent)
    , m_client(std::make_unique<appcomm::client::AppcommClient>(config, this))
{
    connect(m_client.get(), &appcomm::client::AppcommClient::authenticationStateChanged,
            this, &DemoController::onAuthenticationStateChanged);

    connect(m_client.get(), &appcomm::client::AppcommClient::connectionStateChanged,
            this, &DemoController::onConnectionStateChanged);

    connect(m_client.get(), &appcomm::client::AppcommClient::joinedChannel,
            this, &DemoController::onJoinedChannel);

    connect(m_client.get(), &appcomm::client::AppcommClient::messageReceived,
            this, &DemoController::onMessageReceived);

    connect(m_client.get(), &appcomm::client::AppcommClient::errorOccurred,
            this, &DemoController::onErrorOccurred);
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

bool DemoController::busy() const
{
    return m_busy;
}

QString DemoController::userId() const
{
    return m_userId;
}

MessageListModel* DemoController::messagesModel()
{
    return &m_messagesModel;
}

void DemoController::loginAsGuest()
{
    qDebug() << "[DemoController] loginAsGuest called";
    clearError();

    m_loginMode = LoginMode::Guest;
    m_chatOpened = false;
    m_messagesModel.clear();

    setBusy(true);
    m_client->createGuestSession();
}

void DemoController::loginWithEmail(const QString &email, const QString &password)
{
    qDebug() << "[DemoController] loginWithEmail called";
    const QString trimmedEmail = email.trimmed();

    if (trimmedEmail.isEmpty()) {
        setErrorMessage("Email cannot be empty.");
        return;
    }

    if (password.isEmpty()) {
        setErrorMessage("Password cannot be empty.");
        return;
    }

    clearError();

    m_loginMode = LoginMode::Email;
    m_chatOpened = false;
    m_messagesModel.clear();

    setBusy(true);
    m_client->createEmailSession(trimmedEmail, password);
}

void DemoController::sendMessage(const QString &text)
{
    const QString trimmed = text.trimmed();

    if (trimmed.isEmpty()) {
        return;
    }

    if (!m_authenticated) {
        setErrorMessage("You must login before sending messages.");
        return;
    }

    if (m_currentChannel.isEmpty()) {
        setErrorMessage("No channel assigned to this user.");
        return;
    }

    QJsonObject payload;
    payload.insert("text", trimmed);

    clearError();
    m_client->sendMessage(payload);
}

void DemoController::logout()
{
    m_client->logout();

    m_loginMode = LoginMode::None;
    m_chatOpened = false;

    setAuthenticated(false);
    setCurrentChannel(QString());
    setConnectionState("Disconnected");
    setUserId(QString());
    setBusy(false);
    clearError();

    m_messagesModel.clear();
}

void DemoController::clearError()
{
    setErrorMessage(QString());
}

void DemoController::onAuthenticationStateChanged()
{
    const bool isAuthenticated = m_client->isAuthenticated();
    setAuthenticated(isAuthenticated);

    if (isAuthenticated) {
        setUserId(m_client->sessionInfo().userId);
    } else {
        setUserId(QString());
    }
}

void DemoController::onConnectionStateChanged()
{
    setConnectionState(m_client->connectionStateText());
}

void DemoController::onJoinedChannel(const QString &channelId)
{
    setCurrentChannel(channelId);
    setBusy(false);

    if (m_loginMode == LoginMode::Email && !m_chatOpened) {
        m_chatOpened = true;
        emit loginSucceeded();
    }
}

void DemoController::onMessageReceived(const appcomm::model::Message &message)
{
    ChatMessageItem item;
    item.sender = message.senderId;
    item.body = extractMessageText(message);
    item.timestamp = message.timestamp.toLocalTime().toString("HH:mm");
    item.system = false;
    item.mine = message.senderId == m_client->sessionInfo().userId;

    if (item.body.trimmed().isEmpty()) {
        item.body = "[empty message]";
    }

    m_messagesModel.addMessage(item);
}

void DemoController::onErrorOccurred(const QString &error)
{
    setBusy(false);

    if (m_loginMode == LoginMode::Guest &&
        error.contains("No membership found", Qt::CaseInsensitive)) {
        emit guestAccessDenied();
        return;
    }

    setErrorMessage(error);
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

void DemoController::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }

    m_busy = busy;
    emit busyChanged();
}

void DemoController::setUserId(const QString &userId)
{
    if (m_userId == userId) {
        return;
    }

    m_userId = userId;
    emit userIdChanged();
}

QString DemoController::extractMessageText(const appcomm::model::Message &message) const
{
    const QJsonValue textValue = message.payload.value("text");

    if (textValue.isString()) {
        return textValue.toString();
    }

    return QString::fromUtf8(
        QJsonDocument(message.payload).toJson(QJsonDocument::Compact)
        );
}

void DemoController::addSystemMessage(const QString &text)
{
    m_messagesModel.addMessage({
        "system",
        text,
        QDateTime::currentDateTime().toString("HH:mm"),
        true,
        false
    });
}