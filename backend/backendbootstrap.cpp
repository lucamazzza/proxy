#include "backendbootstrap.h"

#include <QEventLoop>
#include <QTimer>

#include <algorithm>
#include <utility>

using namespace backend;

namespace {

void waitMilliseconds(int milliseconds) {
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

QJsonArray collectionPermissions(bool guestAccessEnabled) {
    if (guestAccessEnabled) {
        return {
            "read(\"any\")",
            "create(\"any\")",
            "update(\"any\")",
            "delete(\"any\")"
        };
    }
    return {
        "read(\"users\")",
        "create(\"users\")",
        "update(\"users\")",
        "delete(\"users\")"
    };
}

}

BackendBootstrapper::BackendBootstrapper(const BackendConfig &config,
                                         appwritesdk::Server *server,
                                         RequestRunner requestRunner)
    : m_config(config)
    , m_server(server)
    , m_requestRunner(std::move(requestRunner))
{
}

BackendConfig BackendBootstrapper::resolvedConfig() const {
    return m_config;
}

QString BackendBootstrapper::toError(const QString &operation, const BackendRequestResult &result) {
    return QString("%1 failed (code %2): %3").arg(operation).arg(result.code).arg(result.message);
}

bool BackendBootstrapper::isAlreadyExists(const BackendRequestResult &result) const {
    if (result.success) {
        return false;
    }
    if (result.code == 409) {
        return true;
    }
    const QString message = result.message.toLower();
    return message.contains("already exists") || message.contains("duplicate");
}

bool BackendBootstrapper::isBootstrapLimitError(const BackendRequestResult &result) const {
    if (result.success) {
        return false;
    }
    const QString message = result.message.toLower();
    return (message.contains("maximum number or size of attributes")
            || message.contains("maximum number of attributes")
            || message.contains("maximum number of indexes")
            || message.contains("maximum number of index"))
        && message.contains("reached");
}

bool BackendBootstrapper::shouldRetryIndexCreation(const BackendRequestResult &result) const {
    if (result.success || isAlreadyExists(result)) {
        return false;
    }
    const QString message = result.message.toLower();
    return message.contains("not yet available")
        || (message.contains("attribute") && message.contains("available"));
}

bool BackendBootstrapper::isDatabaseLimitReached(const BackendRequestResult &result) const {
    if (result.success) {
        return false;
    }
    const QString message = result.message.toLower();
    return message.contains("maximum number of databases")
        || message.contains("max databases")
        || message.contains("only one database")
        || message.contains("one database");
}

appwritesdk::ConnectionConfig BackendBootstrapper::configForCollection(const QString &collectionId) const {
    appwritesdk::ConnectionConfig sdkConfig;
    sdkConfig.endpoint = m_config.endpoint;
    sdkConfig.projectId = m_config.projectId;
    sdkConfig.apiKey = m_config.apiKey;
    sdkConfig.dbId = m_config.databaseId;
    sdkConfig.collectionId = collectionId;
    return sdkConfig;
}

bool BackendBootstrapper::adoptExistingDatabase(QString *errorMessage) {
    const appwritesdk::ConnectionConfig sdkConfig = configForCollection(m_config.messagesCollectionId);
    const BackendRequestResult listResult = m_requestRunner([&]() {
        m_server->listDatabases(sdkConfig);
    }, 30000);
    if (!listResult.success) {
        if (errorMessage) {
            *errorMessage = toError("listDatabases", listResult);
        }
        return false;
    }
    const QJsonArray databases = listResult.data.value("databases").toArray();
    if (databases.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "No existing database found to reuse";
        }
        return false;
    }
    QString existingDatabaseId;
    for (const QJsonValue &databaseValue : databases) {
        const QString databaseId = databaseValue.toObject().value("$id").toString().trimmed();
        if (!databaseId.isEmpty()) {
            existingDatabaseId = databaseId;
            break;
        }
    }
    if (existingDatabaseId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Unable to resolve existing database ID from Appwrite response";
        }
        return false;
    }
    m_config.databaseId = existingDatabaseId;
    return true;
}

bool BackendBootstrapper::ensureCollection(const QString &collectionId,
                                           const QString &name,
                                           const QJsonArray &permissions,
                                           const QList<AttributeDef> &attributes,
                                           const QList<IndexDef> &indexes,
                                           QString *errorMessage) {
    const appwritesdk::ConnectionConfig sdkConfig = configForCollection(collectionId);
    const BackendRequestResult collectionResult = m_requestRunner([&]() {
        m_server->createCollection(sdkConfig, name, permissions);
    }, 30000);
    if (!collectionResult.success
        && !isAlreadyExists(collectionResult)
        && collectionResult.code != 408) {
        if (errorMessage) {
            *errorMessage = toError(QString("createCollection(%1)").arg(collectionId), collectionResult);
        }
        return false;
    }
    for (const AttributeDef &attribute : attributes) {
        const BackendRequestResult attributeResult = m_requestRunner([&]() {
            m_server->createAttribute(
                sdkConfig,
                attribute.type,
                attribute.key,
                attribute.required,
                attribute.options);
        }, 30000);
        if (!attributeResult.success
            && !isAlreadyExists(attributeResult)
            && !isBootstrapLimitError(attributeResult)
            && attributeResult.code != 408) {
            if (errorMessage) {
                *errorMessage = toError(QString("createAttribute(%1.%2)").arg(collectionId, attribute.key),
                                        attributeResult);
            }
            return false;
        }
    }
    for (const IndexDef &index : indexes) {
        constexpr int maxAttempts = 12;
        BackendRequestResult indexResult;
        bool indexCreated = false;
        for (int attempt = 0; attempt < maxAttempts; ++attempt) {
            indexResult = m_requestRunner([&]() {
                m_server->createIndex(
                    sdkConfig,
                    index.key,
                    index.type,
                    index.attributes);
            }, 30000);
            if (indexResult.success
                || isAlreadyExists(indexResult)
                || isBootstrapLimitError(indexResult)) {
                indexCreated = true;
                break;
            }
            if (!shouldRetryIndexCreation(indexResult)) {
                break;
            }
            const int retryDelayMs = std::min(2000, 500 + (attempt * 200));
            waitMilliseconds(retryDelayMs);
        }
        if (!indexCreated) {
            if (shouldRetryIndexCreation(indexResult) || indexResult.code == 408) {
                continue;
            }
            if (errorMessage) {
                *errorMessage = toError(QString("createIndex(%1.%2)").arg(collectionId, index.key),
                                        indexResult);
            }
            return false;
        }
    }
    return true;
}

bool BackendBootstrapper::bootstrap(QString *errorMessage) {
    if (!m_server || !m_requestRunner) {
        if (errorMessage) {
            *errorMessage = "Internal error: bootstrap dependencies are not ready";
        }
        return false;
    }

    appwritesdk::ConnectionConfig sdkConfig = configForCollection(m_config.messagesCollectionId);
    const BackendRequestResult databaseResult = m_requestRunner([&]() {
        m_server->createDatabase(sdkConfig, m_config.databaseId);
    }, 30000);
    if (!databaseResult.success
        && !isAlreadyExists(databaseResult)
        && databaseResult.code != 408) {
        if (isDatabaseLimitReached(databaseResult)) {
            if (!adoptExistingDatabase(errorMessage)) {
                return false;
            }
        } else {
            if (errorMessage) {
                *errorMessage = toError("createDatabase", databaseResult);
            }
            return false;
        }
    }
    const QJsonArray collectionPermissions = ::collectionPermissions(m_config.guestAccessEnabled);
    const QList<AttributeDef> messageAttributes = {
        {"string", "channelId", true, QJsonObject{{"size", 128}}},
        {"string", "senderId", true, QJsonObject{{"size", 128}}},
        {"string", "messageId", true, QJsonObject{{"size", 128}}},
        {"integer", "sequenceNumber", true, QJsonObject{{"min", 0}}},
        {"datetime", "timestamp", true, QJsonObject()},
        {"string", "payload", true, QJsonObject{{"size", 10000}}},
        {"boolean", "isEcho", true, QJsonObject()}
    };
    const QList<IndexDef> messageIndexes = {
        {"idx_channel", "key", {"channelId"}},
        {"idx_timestamp", "key", {"timestamp"}},
        {"idx_sequence", "key", {"sequenceNumber"}},
        {"idx_message_unique", "unique", {"messageId"}},
        {"idx_channel_sequence_unique", "unique", {"channelId", "sequenceNumber"}},
        {"idx_channel_timestamp", "key", {"channelId", "timestamp"}}
    };
    if (!ensureCollection(m_config.messagesCollectionId,
                          "messages",
                          collectionPermissions,
                          messageAttributes,
                          messageIndexes,
                          errorMessage)) {
        return false;
    }
    const QList<AttributeDef> memberAttributes = {
        {"string", "channelId", true, QJsonObject{{"size", 128}}},
        {"string", "userId", true, QJsonObject{{"size", 128}}},
        {"string", "displayName", true, QJsonObject{{"size", 256}}},
        {"datetime", "joinedAt", true, QJsonObject()},
        {"datetime", "lastSeenAt", true, QJsonObject()},
        {"boolean", "isActive", true, QJsonObject()}
    };
    const QList<IndexDef> memberIndexes = {
        {"idx_member_channel", "key", {"channelId"}},
        {"idx_member_unique", "unique", {"channelId", "userId"}}
    };
    if (!ensureCollection(m_config.membersCollectionId,
                          "members",
                          collectionPermissions,
                          memberAttributes,
                          memberIndexes,
                          errorMessage)) {
        return false;
    }
    const QList<AttributeDef> channelAttributes = {
        {"string", "channelId", true, QJsonObject{{"size", 128}}},
        {"string", "name", true, QJsonObject{{"size", 256}}},
        {"datetime", "createdAt", true, QJsonObject()}
    };
    const QList<IndexDef> channelIndexes = {
        {"idx_channel_id", "unique", {"channelId"}}
    };
    if (!ensureCollection(m_config.channelsCollectionId,
                          "channels",
                          collectionPermissions,
                          channelAttributes,
                          channelIndexes,
                          errorMessage)) {
        return false;
    }
    const QList<AttributeDef> sessionAttributes = {
        {"string", "sessionId", true, QJsonObject{{"size", 128}}},
        {"string", "channelId", true, QJsonObject{{"size", 128}}},
        {"string", "userIds", true, QJsonObject{{"size", 128}, {"array", true}}},
        {"datetime", "createdAt", true, QJsonObject()},
        {"string", "status", true, QJsonObject{{"size", 16}}}
    };
    const QList<IndexDef> sessionIndexes = {
        {"idx_session_id", "unique", {"sessionId"}},
        {"idx_session_channel", "key", {"channelId"}}
    };
    return ensureCollection(m_config.sessionsCollectionId,
                            "sessions",
                            collectionPermissions,
                            sessionAttributes,
                            sessionIndexes,
                            errorMessage);
}
