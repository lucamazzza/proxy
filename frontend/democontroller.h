#ifndef DEMOCONTROLLER_H
#define DEMOCONTROLLER_H

#include <QObject>
#include <QString>
#include <memory>

#include "../appcomm/appcommclient.h"
#include "../appcomm/model.h"
#include "messagelistmodel.h"

class DemoController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString currentChannel READ currentChannel NOTIFY currentChannelChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString userId READ userId NOTIFY userIdChanged)
    Q_PROPERTY(MessageListModel* messagesModel READ messagesModel CONSTANT)

public:
    explicit DemoController(const appcomm::model::AppCommConfig &config,
                            QObject *parent = nullptr);

    QString connectionState() const;
    QString currentChannel() const;
    QString errorMessage() const;
    bool authenticated() const;
    bool busy() const;
    QString userId() const;
    MessageListModel* messagesModel();

    Q_INVOKABLE void loginAsGuest();
    Q_INVOKABLE void loginWithEmail(const QString &email, const QString &password);
    Q_INVOKABLE void sendMessage(const QString &text);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void clearError();

signals:
    void connectionStateChanged();
    void currentChannelChanged();
    void errorMessageChanged();
    void authenticatedChanged();
    void busyChanged();
    void userIdChanged();

    void loginSucceeded();
    void guestAccessDenied();

private slots:
    void onAuthenticationStateChanged();
    void onConnectionStateChanged();
    void onJoinedChannel(const QString &channelId);
    void onMessageReceived(const appcomm::model::Message &message);
    void onErrorOccurred(const QString &error);

private:
    enum class LoginMode {
        None = 0,
        Guest,
        Email
    };

    void setConnectionState(const QString &state);
    void setCurrentChannel(const QString &channel);
    void setErrorMessage(const QString &message);
    void setAuthenticated(bool authenticated);
    void setBusy(bool busy);
    void setUserId(const QString &userId);

    QString extractMessageText(const appcomm::model::Message &message) const;
    void addSystemMessage(const QString &text);

private:
    std::unique_ptr<appcomm::client::AppcommClient> m_client;

    QString m_connectionState = "Disconnected";
    QString m_currentChannel;
    QString m_errorMessage;
    QString m_userId;

    bool m_authenticated = false;
    bool m_busy = false;
    bool m_chatOpened = false;

    LoginMode m_loginMode = LoginMode::None;

    MessageListModel m_messagesModel;
};

#endif // DEMOCONTROLLER_H