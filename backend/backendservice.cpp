#include "backendservice.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>
#include <QTimer>
#include <QUuid>

#include <algorithm>

#include "model.h"
#include "realtime.h"

using namespace backend;

namespace {

QString escapeQueryValue(const QString &value) {
    QString escaped = value;
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");
    return escaped;
}

QString unescapeQueryValue(const QString &value) {
    QString unescaped = value;
    unescaped.replace("\\\"", "\"");
    unescaped.replace("\\\\", "\\");
    return unescaped;
}

QString equalQuery(const QString &key, const QString &value) {
    return QString("equal(\"%1\",[\"%2\"])").arg(key, escapeQueryValue(value));
}

QJsonArray toJsonArray(const QStringList &values) {
    QJsonArray array;
    for (const QString &value : values) {
        array.append(value);
    }
    return array;
}

QStringList deduplicateUsers(const QStringList &users) {
    QSet<QString> seen;
    QStringList deduplicated;
    for (const QString &rawUserId : users) {
        const QString userId = rawUserId.trimmed();
        if (userId.isEmpty() || seen.contains(userId)) {
            continue;
        }
        seen.insert(userId);
        deduplicated.append(userId);
    }
    return deduplicated;
}

bool isAlreadyExistsError(const BackendRequestResult &result) {
    if (result.code == 409) {
        return true;
    }
    const QString message = result.message.toLower();
    return message.contains("already exists") || message.contains("duplicate");
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

bool Session::isValid() const {
    const QString normalizedStatus = status.trimmed().toLower();
    return !sessionId.trimmed().isEmpty()
        && !channelId.trimmed().isEmpty()
        && createdAt.isValid()
        && (normalizedStatus == "open" || normalizedStatus == "closed");
}

QJsonObject Session::toJson() const {
    QJsonObject obj;
    obj["sessionId"] = sessionId;
    obj["channelId"] = channelId;
    obj["userIds"] = toJsonArray(userIds);
    obj["createdAt"] = createdAt.toString(Qt::ISODate);
    obj["status"] = status;
    return obj;
}

Session Session::fromJson(const QJsonObject &obj) {
    Session session;
    const QJsonValue sessionIdValue = obj.value("sessionId");
    const QJsonValue channelIdValue = obj.value("channelId");
    const QJsonValue userIdsValue = obj.value("userIds");
    const QJsonValue createdAtValue = obj.value("createdAt");
    const QJsonValue statusValue = obj.value("status");
    if (!sessionIdValue.isString()
        || !channelIdValue.isString()
        || !userIdsValue.isArray()
        || !createdAtValue.isString()
        || !statusValue.isString()) {
        return {};
    }
    session.sessionId = sessionIdValue.toString().trimmed();
    session.channelId = channelIdValue.toString().trimmed();
    const QJsonArray userIdsArray = userIdsValue.toArray();
    for (const QJsonValue &userIdValue : userIdsArray) {
        if (userIdValue.isString()) {
            session.userIds.append(userIdValue.toString().trimmed());
        }
    }
    session.userIds = deduplicateUsers(session.userIds);
    session.createdAt = QDateTime::fromString(createdAtValue.toString(), Qt::ISODate);
    session.status = statusValue.toString().trimmed().toLower();
    return session;
}

BackendService::BackendService(const BackendConfig &config, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_server(&m_network, this)
{
}

BackendConfig BackendService::resolvedConfig() const {
    return m_config;
}

BackendRequestResult BackendService::performRequest(const std::function<void()> &call, int timeoutMs) {
    BackendRequestResult result;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    const QMetaObject::Connection successConnection = QObject::connect(
        &m_server,
        &appwritesdk::Server::requestSuccess,
        &loop,
        [&](const QJsonObject &data) {
            if (result.completed) {
                return;
            }
            result.completed = true;
            result.success = true;
            result.data = data;
            loop.quit();
        });
    const QMetaObject::Connection errorConnection = QObject::connect(
        &m_server,
        &appwritesdk::Server::requestError,
        &loop,
        [&](int code, const QString &message) {
            if (result.completed) {
                return;
            }
            result.completed = true;
            result.success = false;
            result.code = code;
            result.message = message;
            loop.quit();
        });
    const QMetaObject::Connection timeoutConnection = QObject::connect(
        &timer,
        &QTimer::timeout,
        &loop,
        [&]() {
            if (result.completed) {
                return;
            }
            result.completed = true;
            result.success = false;
            result.code = 408;
            result.message = "Timeout waiting for Appwrite response";
            loop.quit();
        });
    timer.start(timeoutMs);
    call();
    loop.exec();
    QObject::disconnect(successConnection);
    QObject::disconnect(errorConnection);
    QObject::disconnect(timeoutConnection);
    if (!result.completed) {
        result.completed = true;
        result.success = false;
        result.code = 500;
        result.message = "Internal error waiting for Appwrite response";
    }
    return result;
}

QString BackendService::makeUuid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString BackendService::toError(const QString &operation, const BackendRequestResult &result) {
    return QString("%1 failed (code %2): %3").arg(operation).arg(result.code).arg(result.message);
}

appwritesdk::ConnectionConfig BackendService::configForCollection(const QString &collectionId) const {
    appwritesdk::ConnectionConfig sdkConfig;
    sdkConfig.endpoint = m_config.endpoint;
    sdkConfig.projectId = m_config.projectId;
    sdkConfig.apiKey = m_config.apiKey;
    sdkConfig.dbId = m_config.databaseId;
    sdkConfig.collectionId = collectionId;
    return sdkConfig;
}

bool BackendService::bootstrap(QString *errorMessage) {
    BackendBootstrapper bootstrapper(
        m_config,
        &m_server,
        [this](const std::function<void()> &call, int timeoutMs) {
            return performRequest(call, timeoutMs);
        });
    if (!bootstrapper.bootstrap(errorMessage)) {
        return false;
    }
    m_config = bootstrapper.resolvedConfig();
    return true;
}

bool BackendService::listAllDocuments(const QString &collectionId,
                                      QJsonArray *documents,
                                      QString *errorMessage) {
    if (!documents) {
        if (errorMessage) {
            *errorMessage = "Internal error: documents output pointer is null";
        }
        return false;
    }
    QJsonArray allDocuments;
    int offset = 0;
    constexpr int pageSize = 100;
    const appwritesdk::ConnectionConfig sdkConfig = configForCollection(collectionId);
    while (true) {
        const QJsonArray queries = {
            QString("limit(%1)").arg(pageSize),
            QString("offset(%1)").arg(offset)
        };
        const BackendRequestResult result = performRequest([&]() {
            m_server.listDocuments(sdkConfig, queries);
        });
        if (!result.success) {
            if (errorMessage) {
                *errorMessage = toError(QString("listDocuments(%1)").arg(collectionId), result);
            }
            return false;
        }
        const QJsonArray page = result.data.value("documents").toArray();
        for (const QJsonValue &doc : page) {
            allDocuments.append(doc);
        }
        if (page.isEmpty()) {
            break;
        }
        offset += page.size();
        const int total = result.data.value("total").toInt(-1);
        if ((total >= 0 && allDocuments.size() >= total) || page.size() < pageSize) {
            break;
        }
    }
    *documents = allDocuments;
    return true;
}

bool BackendService::listAllUsers(QJsonArray *users, QString *errorMessage) {
    if (!users) {
        if (errorMessage) {
            *errorMessage = "Internal error: users output pointer is null";
        }
        return false;
    }
    QJsonArray allUsers;
    int offset = 0;
    constexpr int pageSize = 100;
    appwritesdk::ConnectionConfig sdkConfig;
    sdkConfig.endpoint = m_config.endpoint;
    sdkConfig.projectId = m_config.projectId;
    sdkConfig.apiKey = m_config.apiKey;
    while (true) {
        const QJsonArray queries = {
            QString("limit(%1)").arg(pageSize),
            QString("offset(%1)").arg(offset)
        };
        const BackendRequestResult result = performRequest([&]() {
            m_server.listUsers(sdkConfig, queries);
        });
        if (!result.success) {
            if (errorMessage) {
                *errorMessage = toError("listUsers", result);
            }
            return false;
        }
        const QJsonArray page = result.data.value("users").toArray();
        for (const QJsonValue &user : page) {
            allUsers.append(user);
        }
        if (page.isEmpty()) {
            break;
        }
        offset += page.size();
        const int total = result.data.value("total").toInt(-1);
        if ((total >= 0 && allUsers.size() >= total) || page.size() < pageSize) {
            break;
        }
    }
    *users = allUsers;
    return true;
}

bool BackendService::listDocuments(const QString &collectionId,
                                   const QJsonArray &queries,
                                   QJsonArray *documents,
                                   QString *errorMessage) {
    if (!documents) {
        if (errorMessage) {
            *errorMessage = "Internal error: documents output pointer is null";
        }
        return false;
    }

    const auto runLegacyFiltering = [&]() -> bool {
        QJsonArray allDocuments;
        if (!listAllDocuments(collectionId, &allDocuments, errorMessage)) {
            return false;
        }

        QList<QPair<QString, QString>> equalsFilters;
        QString orderingField;
        bool descendingOrder = false;
        int limit = -1;
        int offset = 0;
        const QRegularExpression equalArrayRegex(
            "^equal\\(\"([^\"]+)\",\\[\"((?:\\\\.|[^\"])*)\"\\]\\)$");
        const QRegularExpression equalLegacyRegex(
            "^equal\\(\"([^\"]+)\",\"((?:\\\\.|[^\"])*)\"\\)$");
        const QRegularExpression orderAscRegex("^orderAsc\\(\"([^\"]+)\"\\)$");
        const QRegularExpression orderDescRegex("^orderDesc\\(\"([^\"]+)\"\\)$");
        const QRegularExpression limitRegex("^limit\\((\\d+)\\)$");
        const QRegularExpression offsetRegex("^offset\\((\\d+)\\)$");

        for (const QJsonValue &queryValue : queries) {
            const QString query = queryValue.toString().trimmed();
            if (query.isEmpty()) {
                continue;
            }
            QRegularExpressionMatch match = equalArrayRegex.match(query);
            if (match.hasMatch()) {
                equalsFilters.append(qMakePair(match.captured(1), unescapeQueryValue(match.captured(2))));
                continue;
            }
            match = equalLegacyRegex.match(query);
            if (match.hasMatch()) {
                equalsFilters.append(qMakePair(match.captured(1), unescapeQueryValue(match.captured(2))));
                continue;
            }
            match = orderAscRegex.match(query);
            if (match.hasMatch()) {
                orderingField = match.captured(1);
                descendingOrder = false;
                continue;
            }
            match = orderDescRegex.match(query);
            if (match.hasMatch()) {
                orderingField = match.captured(1);
                descendingOrder = true;
                continue;
            }
            match = limitRegex.match(query);
            if (match.hasMatch()) {
                limit = match.captured(1).toInt();
                continue;
            }
            match = offsetRegex.match(query);
            if (match.hasMatch()) {
                offset = match.captured(1).toInt();
                continue;
            }
        }

        QList<QJsonObject> filteredDocuments;
        filteredDocuments.reserve(allDocuments.size());
        for (const QJsonValue &docValue : allDocuments) {
            filteredDocuments.append(docValue.toObject());
        }
        for (const auto &equalsFilter : equalsFilters) {
            const QString field = equalsFilter.first;
            const QString expectedValue = equalsFilter.second;
            QList<QJsonObject> remainingDocuments;
            remainingDocuments.reserve(filteredDocuments.size());
            for (const QJsonObject &doc : filteredDocuments) {
                const QJsonValue value = doc.value(field);
                if (value.isString()) {
                    if (value.toString() == expectedValue) {
                        remainingDocuments.append(doc);
                    }
                } else if (value.isDouble()) {
                    if (QString::number(value.toDouble()) == expectedValue) {
                        remainingDocuments.append(doc);
                    }
                } else if (value.isBool()) {
                    const QString booleanValue = value.toBool() ? "true" : "false";
                    if (booleanValue == expectedValue.toLower()) {
                        remainingDocuments.append(doc);
                    }
                }
            }
            filteredDocuments = remainingDocuments;
        }
        if (!orderingField.trimmed().isEmpty()) {
            std::sort(filteredDocuments.begin(),
                      filteredDocuments.end(),
                      [&](const QJsonObject &left, const QJsonObject &right) {
                          const QJsonValue leftValue = left.value(orderingField);
                          const QJsonValue rightValue = right.value(orderingField);
                          if (leftValue.isDouble() && rightValue.isDouble()) {
                              if (descendingOrder) {
                                  return leftValue.toDouble() > rightValue.toDouble();
                              }
                              return leftValue.toDouble() < rightValue.toDouble();
                          }
                          const QString leftString = leftValue.toString();
                          const QString rightString = rightValue.toString();
                          if (descendingOrder) {
                              return leftString > rightString;
                          }
                          return leftString < rightString;
                      });
        }

        if (offset > 0) {
            if (offset >= filteredDocuments.size()) {
                filteredDocuments.clear();
            } else {
                filteredDocuments = filteredDocuments.mid(offset);
            }
        }
        if (limit >= 0 && filteredDocuments.size() > limit) {
            filteredDocuments = filteredDocuments.mid(0, limit);
        }

        QJsonArray outputDocuments;
        for (const QJsonObject &doc : filteredDocuments) {
            outputDocuments.append(doc);
        }
        *documents = outputDocuments;
        return true;
    };

    int requestedLimit = -1;
    int baseOffset = 0;
    QJsonArray baseQueries;
    const QRegularExpression limitRegex("^limit\\((\\d+)\\)$");
    const QRegularExpression offsetRegex("^offset\\((\\d+)\\)$");

    for (const QJsonValue &queryValue : queries) {
        const QString query = queryValue.toString().trimmed();
        if (query.isEmpty()) {
            continue;
        }
        QRegularExpressionMatch match = limitRegex.match(query);
        if (match.hasMatch()) {
            requestedLimit = match.captured(1).toInt();
            continue;
        }
        match = offsetRegex.match(query);
        if (match.hasMatch()) {
            baseOffset = match.captured(1).toInt();
            continue;
        }
        baseQueries.append(query);
    }

    if (requestedLimit == 0) {
        *documents = QJsonArray();
        return true;
    }

    QJsonArray collectedDocuments;
    const int initialOffset = std::max(0, baseOffset);
    int currentOffset = initialOffset;
    int remaining = requestedLimit;
    constexpr int maxPageSize = 100;
    const appwritesdk::ConnectionConfig sdkConfig = configForCollection(collectionId);

    while (true) {
        int pageSize = maxPageSize;
        if (remaining > 0) {
            pageSize = std::min(pageSize, remaining);
        }

        QJsonArray pageQueries = baseQueries;
        pageQueries.append(QString("limit(%1)").arg(pageSize));
        pageQueries.append(QString("offset(%1)").arg(currentOffset));

        const BackendRequestResult result = performRequest([&]() {
            m_server.listDocuments(sdkConfig, pageQueries);
        });
        if (!result.success) {
            const QString normalizedMessage = result.message.toLower();
            const bool querySyntaxUnsupported =
                normalizedMessage.contains("invalid query")
                || normalizedMessage.contains("syntax error");
            if (querySyntaxUnsupported && currentOffset == initialOffset) {
                return runLegacyFiltering();
            }
            if (errorMessage) {
                *errorMessage = toError(QString("listDocuments(%1)").arg(collectionId), result);
            }
            return false;
        }

        const QJsonArray page = result.data.value("documents").toArray();
        for (const QJsonValue &doc : page) {
            collectedDocuments.append(doc);
        }
        if (page.isEmpty()) {
            break;
        }

        currentOffset += page.size();
        if (remaining > 0) {
            remaining -= page.size();
            if (remaining <= 0) {
                break;
            }
        }

        const int total = result.data.value("total").toInt(-1);
        if ((total >= 0 && currentOffset >= total) || page.size() < pageSize) {
            break;
        }
    }

    *documents = collectedDocuments;
    return true;
}

bool BackendService::createDocument(const QString &collectionId,
                                    const QJsonObject &data,
                                    QJsonObject *responseData,
                                    QString *errorMessage) {
    const appwritesdk::ConnectionConfig sdkConfig = configForCollection(collectionId);
    const BackendRequestResult result = performRequest([&]() {
        m_server.createDocument(sdkConfig, data);
    });
    if (!result.success) {
        if (errorMessage) {
            *errorMessage = toError(QString("createDocument(%1)").arg(collectionId), result);
        }
        return false;
    }
    if (responseData) {
        *responseData = result.data;
    }
    return true;
}

bool BackendService::deleteDocument(const QString &collectionId,
                                    const QString &documentId,
                                    QString *errorMessage) {
    const appwritesdk::ConnectionConfig sdkConfig = configForCollection(collectionId);
    const BackendRequestResult result = performRequest([&]() {
        m_server.deleteDocument(sdkConfig, documentId);
    });
    if (!result.success) {
        if (errorMessage) {
            *errorMessage = toError(QString("deleteDocument(%1/%2)").arg(collectionId, documentId),
                                    result);
        }
        return false;
    }
    return true;
}

bool BackendService::findSingleDocument(const QString &collectionId,
                                        const QJsonArray &queries,
                                        QJsonObject *document,
                                        QString *errorMessage) {
    if (!document) {
        if (errorMessage) {
            *errorMessage = "Internal error: document output pointer is null";
        }
        return false;
    }
    QJsonArray documents;
    if (!listDocuments(collectionId, queries, &documents, errorMessage)) {
        return false;
    }
    if (documents.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Document not found";
        }
        return false;
    }
    *document = documents.first().toObject();
    return true;
}

bool BackendService::ensureChannelExists(const QString &channelId, QString *errorMessage) {
    const QJsonArray queries = {
        equalQuery("channelId", channelId),
        "limit(1)"
    };
    QJsonObject channelDocument;
    if (!findSingleDocument(m_config.channelsCollectionId, queries, &channelDocument, errorMessage)) {
        if (errorMessage
            && errorMessage->contains("Document not found", Qt::CaseInsensitive)) {
            *errorMessage = QString("Channel not found: %1").arg(channelId);
        }
        return false;
    }
    return true;
}

bool BackendService::createChannel(const QString &name,
                                   QJsonObject *createdChannel,
                                   QString *errorMessage) {
    if (name.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Channel name cannot be empty";
        }
        return false;
    }
    for (int attempt = 0; attempt < 5; ++attempt) {
        QJsonObject channel;
        channel["channelId"] = makeUuid();
        channel["name"] = name.trimmed();
        channel["createdAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        QString createError;
        if (createDocument(m_config.channelsCollectionId, channel, nullptr, &createError)) {
            if (createdChannel) {
                *createdChannel = channel;
            }
            return true;
        }
        if (!createError.toLower().contains("already exists")
            && !createError.toLower().contains("duplicate")) {
            if (errorMessage) {
                *errorMessage = createError;
            }
            return false;
        }
    }
    if (errorMessage) {
        *errorMessage = "Unable to generate a unique channel ID";
    }
    return false;
}

bool BackendService::createUser(const QString &email,
                                const QString &password,
                                const QString &name,
                                QJsonObject *createdUser,
                                QString *errorMessage) {
    if (email.trimmed().isEmpty() || password.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Email and password are required";
        }
        return false;
    }
    const appwritesdk::ConnectionConfig sdkConfig = configForCollection(m_config.messagesCollectionId);
    const BackendRequestResult result = performRequest([&]() {
        m_server.createUser(sdkConfig, email.trimmed(), password, name.trimmed());
    });
    if (!result.success) {
        if (errorMessage) {
            *errorMessage = toError("createUser", result);
        }
        return false;
    }
    if (createdUser) {
        QJsonObject user;
        user["userId"] = result.data.value("$id").toString();
        user["email"] = result.data.value("email").toString();
        user["name"] = result.data.value("name").toString();
        *createdUser = user;
    }
    return true;
}

bool BackendService::deleteUser(const QString &userId, QString *errorMessage) {
    if (userId.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = "User ID cannot be empty";
        }
        return false;
    }
    const appwritesdk::ConnectionConfig sdkConfig = configForCollection(m_config.messagesCollectionId);
    const BackendRequestResult result = performRequest([&]() {
        m_server.deleteUser(sdkConfig, userId.trimmed());
    });
    if (!result.success) {
        if (errorMessage) {
            *errorMessage = toError("deleteUser", result);
        }
        return false;
    }
    return true;
}

bool BackendService::listUsers(QJsonArray *users, QString *errorMessage) {
    if (!users) {
        if (errorMessage) {
            *errorMessage = "Internal error: users output pointer is null";
        }
        return false;
    }
    QJsonArray usersArray;
    if (!listAllUsers(&usersArray, errorMessage)) {
        return false;
    }
    QJsonArray normalizedUsers;
    for (const QJsonValue &userValue : usersArray) {
        const QJsonObject userObject = userValue.toObject();
        QJsonObject normalizedUser;
        normalizedUser["userId"] = userObject.value("$id").toString();
        normalizedUser["email"] = userObject.value("email").toString();
        normalizedUser["name"] = userObject.value("name").toString();
        normalizedUsers.append(normalizedUser);
    }
    *users = normalizedUsers;
    return true;
}

bool BackendService::addCollection(const QString &collectionId,
                                   const QString &name,
                                   QJsonObject *createdCollection,
                                   QString *errorMessage) {
    const QString normalizedCollectionId = collectionId.trimmed();
    if (normalizedCollectionId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Collection ID cannot be empty";
        }
        return false;
    }
    const QString normalizedName =
        name.trimmed().isEmpty() ? normalizedCollectionId : name.trimmed();
    const QJsonArray permissions = collectionPermissions(m_config.guestAccessEnabled);
    appwritesdk::ConnectionConfig sdkConfig = configForCollection(normalizedCollectionId);
    const BackendRequestResult result = performRequest([&]() {
        m_server.createCollection(sdkConfig, normalizedName, permissions);
    });
    if (!result.success) {
        if (errorMessage) {
            *errorMessage = toError("createCollection", result);
        }
        return false;
    }
    if (createdCollection) {
        QJsonObject collection;
        collection["collectionId"] = result.data.value("$id").toString();
        collection["name"] = result.data.value("name").toString();
        if (collection.value("collectionId").toString().trimmed().isEmpty()) {
            collection["collectionId"] = normalizedCollectionId;
        }
        if (collection.value("name").toString().trimmed().isEmpty()) {
            collection["name"] = normalizedName;
        }
        *createdCollection = collection;
    }
    return true;
}

bool BackendService::removeCollection(const QString &collectionId, QString *errorMessage) {
    const QString normalizedCollectionId = collectionId.trimmed();
    if (normalizedCollectionId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Collection ID cannot be empty";
        }
        return false;
    }
    appwritesdk::ConnectionConfig sdkConfig = configForCollection(normalizedCollectionId);
    const BackendRequestResult result = performRequest([&]() {
        m_server.deleteCollection(sdkConfig);
    });
    if (!result.success) {
        if (errorMessage) {
            *errorMessage = toError("deleteCollection", result);
        }
        return false;
    }
    return true;
}

bool BackendService::listCollections(QJsonArray *collections, QString *errorMessage) {
    if (!collections) {
        if (errorMessage) {
            *errorMessage = "Internal error: collections output pointer is null";
        }
        return false;
    }
    const appwritesdk::ConnectionConfig sdkConfig = configForCollection(m_config.messagesCollectionId);
    const BackendRequestResult result = performRequest([&]() {
        m_server.listCollections(sdkConfig);
    });
    if (!result.success) {
        if (errorMessage) {
            *errorMessage = toError("listCollections", result);
        }
        return false;
    }
    const QJsonArray collectionsArray = result.data.value("collections").toArray();
    QJsonArray normalizedCollections;
    for (const QJsonValue &collectionValue : collectionsArray) {
        const QJsonObject collectionObject = collectionValue.toObject();
        QJsonObject normalizedCollection;
        normalizedCollection["collectionId"] = collectionObject.value("$id").toString();
        normalizedCollection["name"] = collectionObject.value("name").toString();
        normalizedCollections.append(normalizedCollection);
    }
    *collections = normalizedCollections;
    return true;
}

bool BackendService::listChannels(QJsonArray *channels, QString *errorMessage) {
    if (!channels) {
        if (errorMessage) {
            *errorMessage = "Internal error: channels output pointer is null";
        }
        return false;
    }
    const QJsonArray queries = {
        "orderAsc(\"createdAt\")",
        "limit(1000)"
    };
    QJsonArray channelDocuments;
    if (!listDocuments(m_config.channelsCollectionId, queries, &channelDocuments, errorMessage)) {
        return false;
    }
    QJsonArray result;
    for (const QJsonValue &channelValue : channelDocuments) {
        const QJsonObject channelObject = channelValue.toObject();
        QJsonObject normalizedChannel;
        normalizedChannel["channelId"] = channelObject.value("channelId").toString();
        normalizedChannel["name"] = channelObject.value("name").toString();
        normalizedChannel["createdAt"] = channelObject.value("createdAt").toString();
        result.append(normalizedChannel);
    }
    *channels = result;
    return true;
}

bool BackendService::deleteDocumentsByChannel(const QString &collectionId,
                                              const QString &channelId,
                                              QString *errorMessage) {
    while (true) {
        const QJsonArray queries = {
            equalQuery("channelId", channelId),
            "limit(100)"
        };
        QJsonArray documents;
        if (!listDocuments(collectionId, queries, &documents, errorMessage)) {
            return false;
        }
        if (documents.isEmpty()) {
            return true;
        }
        for (const QJsonValue &documentValue : documents) {
            const QString documentId = documentValue.toObject().value("$id").toString();
            if (documentId.trimmed().isEmpty()) {
                if (errorMessage) {
                    *errorMessage = QString("Unable to resolve document ID while cleaning %1").arg(collectionId);
                }
                return false;
            }
            if (!deleteDocument(collectionId, documentId, errorMessage)) {
                return false;
            }
        }
    }
}

bool BackendService::removeChannel(const QString &channelId, QString *errorMessage) {
    const QString normalizedChannelId = channelId.trimmed();
    if (normalizedChannelId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Channel ID cannot be empty";
        }
        return false;
    }
    QJsonObject channelDocument;
    const QJsonArray channelQueries = {
        equalQuery("channelId", normalizedChannelId),
        "limit(1)"
    };
    if (!findSingleDocument(m_config.channelsCollectionId, channelQueries, &channelDocument, errorMessage)) {
        return false;
    }
    if (!deleteDocumentsByChannel(m_config.membersCollectionId, normalizedChannelId, errorMessage)) {
        return false;
    }
    if (!deleteDocumentsByChannel(m_config.messagesCollectionId, normalizedChannelId, errorMessage)) {
        return false;
    }
    const QString channelDocumentId = channelDocument.value("$id").toString();
    if (channelDocumentId.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Unable to resolve channel document ID";
        }
        return false;
    }
    return deleteDocument(m_config.channelsCollectionId, channelDocumentId, errorMessage);
}

bool BackendService::addMember(const QString &channelId,
                               const QString &userId,
                               const QString &displayName,
                               QJsonObject *member,
                               QString *errorMessage) {
    const QString normalizedChannelId = channelId.trimmed();
    const QString normalizedUserId = userId.trimmed();
    if (normalizedChannelId.isEmpty() || normalizedUserId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Channel ID and User ID are required";
        }
        return false;
    }
    if (!ensureChannelExists(normalizedChannelId, errorMessage)) {
        return false;
    }
    QJsonArray existingQueries = {
        equalQuery("channelId", normalizedChannelId),
        equalQuery("userId", normalizedUserId),
        "limit(1)"
    };
    QJsonArray existingDocuments;
    if (!listDocuments(m_config.membersCollectionId, existingQueries, &existingDocuments, errorMessage)) {
        return false;
    }
    if (!existingDocuments.isEmpty()) {
        const QJsonObject existingMemberObject = existingDocuments.first().toObject();
        const appcomm::model::ChannelMember existingMember =
            appcomm::model::ChannelMember::fromJson(existingMemberObject);
        if (member) {
            if (!existingMember.userId.trimmed().isEmpty()) {
                *member = existingMember.toJson();
            } else {
                *member = existingMemberObject;
            }
        }
        return true;
    }
    appcomm::model::ChannelMember newMember;
    newMember.channelId = normalizedChannelId;
    newMember.userId = normalizedUserId;
    newMember.displayName = displayName.trimmed().isEmpty() ? normalizedUserId : displayName.trimmed();
    newMember.joinedAt = QDateTime::currentDateTimeUtc();
    newMember.lastSeenAt = newMember.joinedAt;
    newMember.isActive = true;
    if (!createDocument(m_config.membersCollectionId, newMember.toJson(), nullptr, errorMessage)) {
        return false;
    }
    if (member) {
        *member = newMember.toJson();
    }
    return true;
}

bool BackendService::removeMember(const QString &channelId,
                                  const QString &userId,
                                  QString *errorMessage)
{
    const QString normalizedChannelId = channelId.trimmed();
    const QString normalizedUserId = userId.trimmed();
    if (normalizedChannelId.isEmpty() || normalizedUserId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Channel ID and User ID are required";
        }
        return false;
    }
    if (!ensureChannelExists(normalizedChannelId, errorMessage)) {
        return false;
    }
    QJsonObject memberDocument;
    const QJsonArray queries = {
        equalQuery("channelId", normalizedChannelId),
        equalQuery("userId", normalizedUserId),
        "limit(1)"
    };
    if (!findSingleDocument(m_config.membersCollectionId, queries, &memberDocument, errorMessage)) {
        return false;
    }
    const QString documentId = memberDocument.value("$id").toString();
    if (documentId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Unable to resolve member document ID";
        }
        return false;
    }
    return deleteDocument(m_config.membersCollectionId, documentId, errorMessage);
}

bool BackendService::listMembers(const QString &channelId,
                                 QJsonArray *members,
                                 QString *errorMessage) {
    if (!members) {
        if (errorMessage) {
            *errorMessage = "Internal error: members output pointer is null";
        }
        return false;
    }
    const QString normalizedChannelId = channelId.trimmed();
    if (normalizedChannelId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Channel ID is required";
        }
        return false;
    }
    if (!ensureChannelExists(normalizedChannelId, errorMessage)) {
        return false;
    }
    const QJsonArray queries = {
        equalQuery("channelId", normalizedChannelId),
        "orderAsc(\"joinedAt\")",
        "limit(1000)"
    };
    QJsonArray memberDocuments;
    if (!listDocuments(m_config.membersCollectionId, queries, &memberDocuments, errorMessage)) {
        return false;
    }
    QJsonArray result;
    for (const QJsonValue &memberValue : memberDocuments) {
        const appcomm::model::ChannelMember parsedMember =
            appcomm::model::ChannelMember::fromJson(memberValue.toObject());
        if (!parsedMember.userId.trimmed().isEmpty()) {
            result.append(parsedMember.toJson());
        }
    }
    *members = result;
    return true;
}

bool BackendService::createSession(const QString &channelId,
                                   const QStringList &userIds,
                                   QJsonObject *createdSession,
                                   QString *errorMessage) {
    const QString normalizedChannelId = channelId.trimmed();
    if (normalizedChannelId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Channel ID is required";
        }
        return false;
    }
    if (!ensureChannelExists(normalizedChannelId, errorMessage)) {
        return false;
    }
    Session session;
    session.sessionId = makeUuid();
    session.channelId = normalizedChannelId;
    session.userIds = deduplicateUsers(userIds);
    session.createdAt = QDateTime::currentDateTimeUtc();
    session.status = "open";
    if (!session.isValid()) {
        if (errorMessage) {
            *errorMessage = "Invalid session data";
        }
        return false;
    }
    if (!createDocument(m_config.sessionsCollectionId, session.toJson(), nullptr, errorMessage)) {
        return false;
    }
    if (createdSession) {
        *createdSession = session.toJson();
    }
    return true;
}

bool BackendService::listSessions(QJsonArray *sessions, QString *errorMessage) {
    if (!sessions) {
        if (errorMessage) {
            *errorMessage = "Internal error: sessions output pointer is null";
        }
        return false;
    }
    const QJsonArray queries = {
        "orderDesc(\"createdAt\")",
        "limit(1000)"
    };
    QJsonArray sessionDocuments;
    if (!listDocuments(m_config.sessionsCollectionId, queries, &sessionDocuments, errorMessage)) {
        return false;
    }
    QList<Session> parsedSessions;
    parsedSessions.reserve(sessionDocuments.size());
    for (const QJsonValue &sessionValue : sessionDocuments) {
        const Session session = Session::fromJson(sessionValue.toObject());
        if (session.isValid()) {
            parsedSessions.append(session);
        }
    }
    std::sort(parsedSessions.begin(),
              parsedSessions.end(),
              [](const Session &left, const Session &right) {
                  return left.createdAt > right.createdAt;
              });
    QJsonArray result;
    for (const Session &session : parsedSessions) {
        result.append(session.toJson());
    }
    *sessions = result;
    return true;
}

bool BackendService::closeSession(const QString &sessionId, QString *errorMessage) {
    const QString normalizedSessionId = sessionId.trimmed();
    if (normalizedSessionId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Session ID is required";
        }
        return false;
    }
    QJsonObject sessionDocument;
    const QJsonArray queries = {
        equalQuery("sessionId", normalizedSessionId),
        "limit(1)"
    };
    if (!findSingleDocument(m_config.sessionsCollectionId, queries, &sessionDocument, errorMessage)) {
        return false;
    }
    const QString documentId = sessionDocument.value("$id").toString();
    if (documentId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Unable to resolve session document ID";
        }
        return false;
    }
    return deleteDocument(m_config.sessionsCollectionId, documentId, errorMessage);
}

bool BackendService::readMessages(const QString &channelId,
                                  const QString &messageId,
                                  int limit,
                                  QJsonArray *messages,
                                  QString *errorMessage) {
    if (!messages) {
        if (errorMessage) {
            *errorMessage = "Internal error: messages output pointer is null";
        }
        return false;
    }
    const QString normalizedChannelId = channelId.trimmed();
    if (normalizedChannelId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Channel ID is required";
        }
        return false;
    }
    if (limit <= 0) {
        if (errorMessage) {
            *errorMessage = "Limit must be a positive integer";
        }
        return false;
    }
    QJsonArray queries;
    queries.append(equalQuery("channelId", normalizedChannelId));
    if (!messageId.trimmed().isEmpty()) {
        queries.append(equalQuery("messageId", messageId.trimmed()));
    }
    queries.append("orderAsc(\"timestamp\")");
    queries.append(QString("limit(%1)").arg(limit));
    QJsonArray messageDocuments;
    if (!listDocuments(m_config.messagesCollectionId, queries, &messageDocuments, errorMessage)) {
        return false;
    }
    QList<appcomm::model::Message> parsedMessages;
    parsedMessages.reserve(messageDocuments.size());
    for (const QJsonValue &messageValue : messageDocuments) {
        const appcomm::model::Message message =
            appcomm::model::Message::fromJson(messageValue.toObject());
        if (message.isValid()) {
            parsedMessages.append(message);
        }
    }
    std::sort(parsedMessages.begin(),
              parsedMessages.end(),
              [](const appcomm::model::Message &left, const appcomm::model::Message &right) {
                  if (left.timestamp == right.timestamp) {
                      return left.sequenceNumber < right.sequenceNumber;
                  }
                  return left.timestamp < right.timestamp;
              });
    QJsonArray result;
    for (const appcomm::model::Message &message : parsedMessages) {
        QJsonObject messageObject;
        messageObject["channelId"] = message.channelId;
        messageObject["senderId"] = message.senderId;
        messageObject["messageId"] = message.messageId;
        messageObject["sequenceNumber"] = message.sequenceNumber;
        messageObject["timestamp"] = message.timestamp.toString(Qt::ISODate);
        messageObject["payload"] = message.payload;
        messageObject["isEcho"] = message.isEcho;
        result.append(messageObject);
    }
    *messages = result;
    return true;
}

bool BackendService::createMessage(const QString &channelId,
                                   const QString &senderId,
                                   const QJsonObject &payload,
                                   QJsonObject *createdMessage,
                                   QString *errorMessage) {
    const QString normalizedChannelId = channelId.trimmed();
    const QString normalizedSenderId = senderId.trimmed();
    if (normalizedChannelId.isEmpty() || normalizedSenderId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Channel ID and sender ID are required";
        }
        return false;
    }
    if (!ensureChannelExists(normalizedChannelId, errorMessage)) {
        return false;
    }
    constexpr int maxSequenceAttempts = 5;
    const appwritesdk::ConnectionConfig sdkConfig = configForCollection(m_config.messagesCollectionId);

    for (int attempt = 0; attempt < maxSequenceAttempts; ++attempt) {
        qint64 nextSequenceNumber = 0;
        const QJsonArray sequenceQueries = {
            equalQuery("channelId", normalizedChannelId),
            "orderDesc(\"sequenceNumber\")",
            "limit(1)"
        };
        QJsonArray latestMessages;
        if (!listDocuments(m_config.messagesCollectionId, sequenceQueries, &latestMessages, errorMessage)) {
            return false;
        }
        if (!latestMessages.isEmpty()) {
            nextSequenceNumber = static_cast<qint64>(
                latestMessages.first().toObject().value("sequenceNumber").toDouble(0.0)) + 1;
        }

        appcomm::model::Message message;
        message.channelId = normalizedChannelId;
        message.senderId = normalizedSenderId;
        message.messageId = makeUuid();
        message.sequenceNumber = nextSequenceNumber;
        message.timestamp = QDateTime::currentDateTimeUtc();
        message.payload = payload;
        message.isEcho = false;

        const BackendRequestResult createResult = performRequest([&]() {
            m_server.createDocument(sdkConfig, message.toJson());
        });
        if (createResult.success) {
            if (createdMessage) {
                QJsonObject outputMessage;
                outputMessage["channelId"] = message.channelId;
                outputMessage["senderId"] = message.senderId;
                outputMessage["messageId"] = message.messageId;
                outputMessage["sequenceNumber"] = message.sequenceNumber;
                outputMessage["timestamp"] = message.timestamp.toString(Qt::ISODate);
                outputMessage["payload"] = message.payload;
                outputMessage["isEcho"] = message.isEcho;
                *createdMessage = outputMessage;
            }
            return true;
        }
        if (isAlreadyExistsError(createResult) && attempt < (maxSequenceAttempts - 1)) {
            continue;
        }
        if (errorMessage) {
            *errorMessage = toError("createMessage", createResult);
        }
        return false;
    }

    if (errorMessage) {
        *errorMessage = "Unable to create message after multiple sequence retries";
    }
    return false;
}

bool BackendService::removeMessage(const QString &messageId, QString *errorMessage) {
    const QString normalizedMessageId = messageId.trimmed();
    if (normalizedMessageId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Message ID cannot be empty";
        }
        return false;
    }
    const QJsonArray queries = {
        equalQuery("messageId", normalizedMessageId),
        "limit(1)"
    };
    QJsonObject messageDocument;
    if (!findSingleDocument(m_config.messagesCollectionId, queries, &messageDocument, errorMessage)) {
        return false;
    }
    const QString documentId = messageDocument.value("$id").toString();
    if (documentId.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Unable to resolve message document ID";
        }
        return false;
    }
    return deleteDocument(m_config.messagesCollectionId, documentId, errorMessage);
}

int BackendService::runEchoService(QString *errorMessage) {
    QCoreApplication *application = QCoreApplication::instance();
    if (!application) {
        if (errorMessage) {
            *errorMessage = "Internal error: QCoreApplication instance is missing";
        }
        return 1;
    }
    QTextStream out(stdout);
    QTextStream err(stderr);
    const appwritesdk::ConnectionConfig realtimeConfig =
        configForCollection(m_config.messagesCollectionId);
    appcomm::Realtime realtime(realtimeConfig, this);
    QObject::connect(&m_server,
                     &appwritesdk::Server::requestError,
                     this,
                     [&err](int code, const QString &message) {
                         err << "Echo publish failed (" << code << "): " << message << Qt::endl;
                     });
    QObject::connect(&m_server,
                     &appwritesdk::Server::requestSuccess,
                     this,
                     [&out](const QJsonObject &data) {
                         if (data.value("isEcho").toBool(false)) {
                             out << "Echo published: " << data.value("messageId").toString() << Qt::endl;
                         }
                     });
    QObject::connect(&realtime, &appcomm::Realtime::connected, this, [&out]() {
        out << "Backend service connected" << Qt::endl;
    });
    QObject::connect(&realtime, &appcomm::Realtime::errorOccurred, this, [&err](const QString &message) {
        err << "Realtime error: " << message << Qt::endl;
    });
    QObject::connect(&realtime, &appcomm::Realtime::eventReceived, this, [&](const QJsonObject &event) {
        QJsonObject eventData = event;
        const QJsonValue dataValue = event.value("data");
        if (dataValue.isObject()) {
            eventData = dataValue.toObject();
        }
        const QJsonValue payloadValue = eventData.value("payload");
        if (!payloadValue.isObject()) {
            return;
        }
        QJsonObject messageObject = payloadValue.toObject();
        if (messageObject.contains("data") && messageObject.value("data").isObject()) {
            messageObject = messageObject.value("data").toObject();
        }
        const appcomm::model::Message incomingMessage =
            appcomm::model::Message::fromJson(messageObject);
        if (!incomingMessage.isValid()) {
            return;
        }
        if (incomingMessage.isEcho) {
            return;
        }
        appcomm::model::Message echoMessage = incomingMessage;
        echoMessage.messageId = makeUuid();
        echoMessage.senderId = "backend-echo";
        if (echoMessage.sequenceNumber < 0) {
            echoMessage.sequenceNumber = 0;
        } else {
            echoMessage.sequenceNumber += 1;
        }
        echoMessage.timestamp = QDateTime::currentDateTimeUtc();
        echoMessage.isEcho = true;
        const appwritesdk::ConnectionConfig sdkConfig = configForCollection(m_config.messagesCollectionId);
        m_server.createDocument(sdkConfig, echoMessage.toJson());
    });
    const QStringList realtimeChannels = {
        QString("databases.%1.collections.%2.documents")
            .arg(m_config.databaseId, m_config.messagesCollectionId)
    };
    realtime.connect(realtimeChannels);
    out << "Running service for all channels" << Qt::endl;
    return application->exec();
}
