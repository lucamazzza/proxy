#include "backendconfig.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>

using namespace backend;

namespace {

QString readString(const QJsonObject &obj, const QString &key, const QString &fallback = QString()) {
    const QJsonValue value = obj.value(key);
    if (!value.isString()) {
        return fallback;
    }
    return value.toString();
}

bool readBool(const QJsonObject &obj, const QString &key, bool fallback = false) {
    const QJsonValue value = obj.value(key);
    if (value.isBool()) {
        return value.toBool();
    }
    if (!value.isString()) {
        return fallback;
    }
    const QString normalized = value.toString().trimmed().toLower();
    if (normalized == "true" || normalized == "1" || normalized == "yes" || normalized == "on") {
        return true;
    }
    if (normalized == "false" || normalized == "0" || normalized == "no" || normalized == "off") {
        return false;
    }
    return fallback;
}

}

bool BackendConfig::isValid() const {
    return !endpoint.trimmed().isEmpty()
        && !projectId.trimmed().isEmpty()
        && !apiKey.trimmed().isEmpty()
        && !databaseId.trimmed().isEmpty()
        && !messagesCollectionId.trimmed().isEmpty()
        && !incomingMessagesCollectionId.trimmed().isEmpty()
        && !membersCollectionId.trimmed().isEmpty()
        && !topicsCollectionId.trimmed().isEmpty()
        && !sessionsCollectionId.trimmed().isEmpty();
}

QJsonObject BackendConfig::toJson() const {
    QJsonObject obj;
    obj["endpoint"] = endpoint;
    obj["projectId"] = projectId;
    obj["apiKey"] = apiKey;
    obj["databaseId"] = databaseId;
    obj["guestAccessEnabled"] = guestAccessEnabled;
    obj["messagesCollectionId"] = messagesCollectionId;
    obj["incomingMessagesCollectionId"] = incomingMessagesCollectionId;
    obj["membersCollectionId"] = membersCollectionId;
    obj["topicsCollectionId"] = topicsCollectionId;
    obj["sessionsCollectionId"] = sessionsCollectionId;
    return obj;
}

BackendConfig BackendConfig::fromJson(const QJsonObject &obj) {
    BackendConfig config;
    config.endpoint = readString(obj, "endpoint");
    config.projectId = readString(obj, "projectId");
    config.apiKey = readString(obj, "apiKey");
    config.databaseId = readString(obj, "databaseId");
    config.guestAccessEnabled = readBool(obj, "guestAccessEnabled", false);
    config.messagesCollectionId = readString(obj, "messagesCollectionId", "messages");
    config.incomingMessagesCollectionId =
        readString(obj, "incomingMessagesCollectionId", "pendingmessages");
    config.membersCollectionId = readString(obj, "membersCollectionId", "members");
    config.topicsCollectionId = readString(obj, "topicsCollectionId", "topics");
    config.sessionsCollectionId = readString(obj, "sessionsCollectionId", "sessions");
    return config;
}

QString backend::defaultConfigPath() {
    const QString overridePath = qEnvironmentVariable("PROXY_BACKEND_CONFIG_PATH").trimmed();
    if (!overridePath.isEmpty()) {
        return overridePath;
    }
    return QDir::homePath() + "/.proxy-backend-config.json";
}

bool backend::saveConfig(const BackendConfig &config, QString *errorMessage) {
    const QString path = defaultConfigPath();
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = QString("Unable to open config file for writing: %1").arg(path);
        }
        return false;
    }
    const QJsonDocument document(config.toJson());
    if (file.write(document.toJson(QJsonDocument::Indented)) < 0) {
        if (errorMessage) {
            *errorMessage = QString("Unable to write config file: %1").arg(path);
        }
        return false;
    }
    return true;
}

bool backend::loadConfig(BackendConfig *config, QString *errorMessage) {
    if (!config) {
        if (errorMessage) {
            *errorMessage = "Internal error: missing config output pointer";
        }
        return false;
    }
    const QString path = defaultConfigPath();
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
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage) {
            *errorMessage = QString("Invalid config JSON (%1)").arg(parseError.errorString());
        }
        return false;
    }
    if (!document.isObject()) {
        if (errorMessage) {
            *errorMessage = "Invalid config format: expected object";
        }
        return false;
    }
    const BackendConfig loaded = BackendConfig::fromJson(document.object());
    if (!loaded.isValid()) {
        if (errorMessage) {
            *errorMessage = "Config is missing required fields";
        }
        return false;
    }
    *config = loaded;
    return true;
}
