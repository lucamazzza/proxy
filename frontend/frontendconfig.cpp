#include "frontendconfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace {

QString readString(const QJsonObject &obj, const QString &key, const QString &fallback = QString())
{
    const QJsonValue value = obj.value(key);
    if (!value.isString()) {
        return fallback;
    }
    return value.toString().trimmed();
}

bool isFrontendConfigValid(const appcomm::model::AppCommConfig &config)
{
    return !config.endpoint.trimmed().isEmpty()
        && !config.projectId.trimmed().isEmpty()
        && !config.databaseId.trimmed().isEmpty()
        && !config.messagesCollectionId.trimmed().isEmpty()
        && !config.membersCollectionId.trimmed().isEmpty()
        && !config.incomingMessagesCollectionId.trimmed().isEmpty();
}

QJsonObject configToJson(const appcomm::model::AppCommConfig &config)
{
    QJsonObject obj;
    obj["endpoint"] = config.endpoint;
    obj["projectId"] = config.projectId;
    obj["databaseId"] = config.databaseId;
    obj["messagesCollectionId"] = config.messagesCollectionId;
    obj["membersCollectionId"] = config.membersCollectionId;
    obj["incomingMessagesCollectionId"] = config.incomingMessagesCollectionId;
    return obj;
}

bool loadConfigFromPath(const QString &path,
                        appcomm::model::AppCommConfig *config,
                        QString *errorMessage)
{
    QFile file(path);
    if (!file.exists()) {
        if (errorMessage) {
            *errorMessage = QString("Config file not found: %1").arg(path);
        }
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QString("Unable to open config file: %1").arg(path);
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = QString("Invalid config JSON in %1").arg(path);
        }
        return false;
    }

    const QJsonObject obj = document.object();
    appcomm::model::AppCommConfig loaded;
    loaded.endpoint = readString(obj, "endpoint");
    loaded.projectId = readString(obj, "projectId");
    loaded.databaseId = readString(obj, "databaseId");
    loaded.messagesCollectionId = readString(obj, "messagesCollectionId", "messages");
    loaded.incomingMessagesCollectionId =
        readString(obj, "incomingMessagesCollectionId", "pendingmessages");
    loaded.membersCollectionId = readString(obj, "membersCollectionId", "members");

    if (!isFrontendConfigValid(loaded)) {
        if (errorMessage) {
            *errorMessage = QString("Frontend config is missing required fields: %1").arg(path);
        }
        return false;
    }

    *config = loaded;
    return true;
}

} // namespace

QString frontend::defaultFrontendConfigPath()
{
    const QString overridePath = qEnvironmentVariable("PROXY_FRONTEND_CONFIG_PATH").trimmed();
    if (!overridePath.isEmpty()) {
        return overridePath;
    }
    return QDir::homePath() + "/.proxy-frontend-config.json";
}

bool frontend::loadFrontendConfig(appcomm::model::AppCommConfig *config, QString *errorMessage)
{
    if (!config) {
        if (errorMessage) {
            *errorMessage = "Internal error: missing frontend config output pointer";
        }
        return false;
    }

    QString frontendError;
    if (loadConfigFromPath(defaultFrontendConfigPath(), config, &frontendError)) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = frontendError;
    }
    return false;
}

bool frontend::saveFrontendConfig(const appcomm::model::AppCommConfig &config, QString *errorMessage)
{
    if (!isFrontendConfigValid(config)) {
        if (errorMessage) {
            *errorMessage = "Frontend config is missing required fields";
        }
        return false;
    }

    const QString path = defaultFrontendConfigPath();
    const QFileInfo fileInfo(path);
    const QString parentDir = fileInfo.absolutePath();
    if (!parentDir.isEmpty() && !QDir().mkpath(parentDir)) {
        if (errorMessage) {
            *errorMessage = QString("Unable to create config directory: %1").arg(parentDir);
        }
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = QString("Unable to open config file for writing: %1").arg(path);
        }
        return false;
    }

    const QJsonDocument document(configToJson(config));
    if (file.write(document.toJson(QJsonDocument::Indented)) < 0) {
        if (errorMessage) {
            *errorMessage = QString("Unable to write config file: %1").arg(path);
        }
        return false;
    }

    return true;
}
