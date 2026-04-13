#ifndef DEMOCONTROLLER_H
#define DEMOCONTROLLER_H

#include <QObject>
#include <QString>

#include "../appcomm/appcommclient.h"
#include "model/MessageListModel.h"

class DemoController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString currentChannel READ currentChannel NOTIFY currentChannelChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(MessageListModel* messagesModel READ messagesModel CONSTANT)

public:
    explicit DemoController(QObject *parent = nullptr);

    QString connectionState() const;
    QString currentChannel() const;
    QString errorMessage() const;
    bool authenticated() const;
    MessageListModel* messagesModel();

    Q_INVOKABLE void loginAsGuest();
    Q_INVOKABLE void loginWithEmail(const QString &email, const QString &password);
    Q_INVOKABLE void joinChannel(const QString &channelId);
    Q_INVOKABLE void leaveChannel();
    Q_INVOKABLE void sendMessage(const QString &text);
    Q_INVOKABLE void clearError();

signals:
    void connectionStateChanged();
    void currentChannelChanged();
    void errorMessageChanged();
    void authenticatedChanged();
    void loginSucceeded();

private:
    void setConnectionState(const QString &state);
    void setCurrentChannel(const QString &channel);
    void setErrorMessage(const QString &message);
    void setAuthenticated(bool authenticated);
    void addSystemMessage(const QString &text);
    void addUserMessage(const QString &sender, const QString &text);

private:
    std::unique_ptr<appcomm::client::AppcommClient> m_client;
    QString m_connectionState = "Disconnected";
    QString m_currentChannel;
    QString m_errorMessage;
    bool m_authenticated = false;
    MessageListModel m_messagesModel;
};

#endif // DEMOCONTROLLER_H