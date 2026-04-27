#ifndef BACKENDCONFIG_H
#define BACKENDCONFIG_H

#include <QJsonObject>
#include <QString>

namespace backend {

struct BackendConfig {
    QString endpoint;
    QString projectId;
    QString apiKey;
    QString databaseId;
    bool guestAccessEnabled = false;
    QString messagesCollectionId = "messages";
    QString incomingMessagesCollectionId = "pendingmessages";
    QString membersCollectionId = "members";
    QString channelsCollectionId = "channels";
    QString sessionsCollectionId = "sessions";

    bool isValid() const;
    QJsonObject toJson() const;
    static BackendConfig fromJson(const QJsonObject &obj);
};

QString defaultConfigPath();
bool saveConfig(const BackendConfig &config, QString *errorMessage);
bool loadConfig(BackendConfig *config, QString *errorMessage);

} // namespace backend

#endif // BACKENDCONFIG_H
