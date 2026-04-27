/*!
 * @file appwritesdk.cpp
 * @brief Module in charge of the connection via HTTP to an Appwrite instance.
 *
 * Provides Qt-based wrapper classes for Appwrite REST API operations.
 * Includes client-side authentication and document operations, as well as
 * server-side administrative functions for database and user management.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#include "appwritesdk.h"
#include <QJsonDocument>
#include <QRegularExpression>
#include <QUrlQuery>
#include <QUrl>

using namespace appwritesdk;

namespace {

QString unescapeQueryValue(const QString &value) {
    QString unescaped = value;
    unescaped.replace("\\\"", "\"");
    unescaped.replace("\\\\", "\\");
    return unescaped;
}

void appendListQueryItem(const QString &rawQuery, QUrlQuery *urlQuery, bool allowFunctionQueries) {
    if (!urlQuery) {
        return;
    }
    const QString query = rawQuery.trimmed();
    if (query.isEmpty()) {
        return;
    }

    static const QRegularExpression limitRegex("^limit\\((\\d+)\\)$");
    static const QRegularExpression offsetRegex("^offset\\((\\d+)\\)$");
    static const QRegularExpression searchRegex("^search\\(\"((?:\\\\.|[^\"])*)\"\\)$");

    QRegularExpressionMatch match = limitRegex.match(query);
    if (match.hasMatch()) {
        urlQuery->addQueryItem("limit", match.captured(1));
        return;
    }
    match = offsetRegex.match(query);
    if (match.hasMatch()) {
        urlQuery->addQueryItem("offset", match.captured(1));
        return;
    }
    match = searchRegex.match(query);
    if (match.hasMatch()) {
        urlQuery->addQueryItem("search", unescapeQueryValue(match.captured(1)));
        return;
    }
    if (allowFunctionQueries) {
        urlQuery->addQueryItem("queries[]", query);
    }
}

}

BaseSDK::BaseSDK(QNetworkAccessManager *mgr, QObject *parent)
    : QObject(parent), m_network(mgr)
{
    if (!m_network->cookieJar()) {
        m_network->setCookieJar(new QNetworkCookieJar(m_network));
    }
}

void BaseSDK::onResponseFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        if (doc.isObject()) {
            emit requestSuccess(doc.object());
        } else if (doc.isArray()) {
            QJsonObject wrapper;
            wrapper["documents"] = doc.array();
            emit requestSuccess(wrapper);
        } else {
            emit requestSuccess(QJsonObject());
        }
    } else {
        parseErrorResponse(reply);
    }
}

void BaseSDK::parseErrorResponse(QNetworkReply *reply) {
    int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QString errorMessage;
    if (doc.isObject()) {
        QJsonObject errObj = doc.object();
        errorMessage = errObj["message"].toString();
        if (errorMessage.isEmpty()) {
            errorMessage = errObj["error"].toString();
        }
    }
    if (errorMessage.isEmpty()) {
        errorMessage = reply->errorString();
    }
    emit requestError(httpCode, errorMessage);
}

QNetworkRequest BaseSDK::createBaseRequest(const ConnectionConfig &config,
                                           const QString &path,
                                           bool isAdmin) {
    QString fullUrl = config.endpoint;
    if (!fullUrl.endsWith('/')) fullUrl += '/';
    if (path.startsWith('/')) fullUrl += path.mid(1);
    else fullUrl += path;
    QNetworkRequest req((QUrl(fullUrl)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("X-Appwrite-Project", config.projectId.toUtf8());
    if (isAdmin && !config.apiKey.isEmpty()) {
        req.setRawHeader("X-Appwrite-Key", config.apiKey.toUtf8());
    }
    return req;
}

void Client::createAnonymousSession(const ConnectionConfig &config) {
    QNetworkRequest req = createBaseRequest(config, "account/sessions/anonymous", APPCOMM_USER);
    QNetworkReply *reply = m_network->post(req, QByteArray());
    connect(reply, &QNetworkReply::finished, this, &Client::onResponseFinished);
}

void Client::createEmailSession(const ConnectionConfig &config,
                                const QString &email,
                                const QString &password) {
    QNetworkRequest req = createBaseRequest(config, "account/sessions/email", APPCOMM_USER);
    QJsonObject body;
    body["email"] = email;
    body["password"] = password;
    QNetworkReply *reply = m_network->post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, &Client::onResponseFinished);
}

void Client::deleteSession(const ConnectionConfig &config, const QString &sessionId) {
    QString path = QString("account/sessions/%1").arg(sessionId);
    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_USER);
    QNetworkReply *reply = m_network->deleteResource(req);
    connect(reply, &QNetworkReply::finished, this, &Client::onResponseFinished);
}

void Client::deleteSessions(const ConnectionConfig &config) {
    QNetworkRequest req = createBaseRequest(config, "account/sessions", APPCOMM_USER);
    QNetworkReply *reply = m_network->deleteResource(req);
    connect(reply, &QNetworkReply::finished, this, &Client::onResponseFinished);
}

void Client::getAccount(const ConnectionConfig &config) {
    QNetworkRequest req = createBaseRequest(config, "account", APPCOMM_USER);
    QNetworkReply *reply = m_network->get(req);
    connect(reply, &QNetworkReply::finished, this, &Client::onResponseFinished);
}

void Client::createDocument(const ConnectionConfig &config, const QJsonObject &data) {
    QString path = QString("databases/%1/collections/%2/documents")
                       .arg(config.dbId, config.collectionId);
    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_USER);
    QJsonObject body;
    body["documentId"] = "unique()";
    body["data"] = data;
    body["permissions"] = QJsonArray{"read(\"any\")", "write(\"any\")"};
    QNetworkReply *reply = m_network->post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, &Client::onResponseFinished);
}

void Client::listDocuments(const ConnectionConfig &config, const QJsonArray &queries) {
    QString path = QString("databases/%1/collections/%2/documents")
    .arg(config.dbId, config.collectionId);

    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_USER);
    QUrl url = req.url();

    if (!queries.isEmpty()) {
        QStringList encodedQueries;

        for (const QJsonValue &q : queries) {
            const QString queryString = q.toString().trimmed();
            if (!queryString.isEmpty()) {
                encodedQueries.append(
                    "queries[]=" + QUrl::toPercentEncoding(queryString)
                    );
            }
        }

        url.setQuery(encodedQueries.join("&"), QUrl::TolerantMode);
    }

    req.setUrl(url);

    qDebug() << "[Client] listDocuments URL:" << url.toString();

    QNetworkReply *reply = m_network->get(req);
    connect(reply, &QNetworkReply::finished, this, &Client::onResponseFinished);
}

void Client::getDocument(const ConnectionConfig &config, const QString &documentId) {
    QString path = QString("databases/%1/collections/%2/documents/%3")
                       .arg(config.dbId, config.collectionId, documentId);
    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_USER);
    QNetworkReply *reply = m_network->get(req);
    connect(reply, &QNetworkReply::finished, this, &Client::onResponseFinished);
}

void Client::updateDocument(const ConnectionConfig &config,
                           const QString &documentId,
                           const QJsonObject &data) {
    QString path = QString("databases/%1/collections/%2/documents/%3")
                       .arg(config.dbId, config.collectionId, documentId);
    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_USER);
    QJsonObject body;
    body["data"] = data;
    QNetworkReply *reply = m_network->put(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, &Client::onResponseFinished);
}

void Client::deleteDocument(const ConnectionConfig &config, const QString &documentId) {
    QString path = QString("databases/%1/collections/%2/documents/%3")
                       .arg(config.dbId, config.collectionId, documentId);
    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_USER);
    QNetworkReply *reply = m_network->deleteResource(req);
    connect(reply, &QNetworkReply::finished, this, &Client::onResponseFinished);
}

void Server::createDatabase(const ConnectionConfig &config, const QString &name) {
    QNetworkRequest req = createBaseRequest(config, "databases", APPCOMM_ADMIN);
    QJsonObject body;
    body["databaseId"] = config.dbId;
    body["name"] = name;
    QNetworkReply *reply = m_network->post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, &Server::onResponseFinished);
}

void Server::deleteDatabase(const ConnectionConfig &config) {
    QString path = QString("databases/%1").arg(config.dbId);
    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_ADMIN);
    QNetworkReply *reply = m_network->deleteResource(req);
    connect(reply, &QNetworkReply::finished, this, &Server::onResponseFinished);
}

void Server::listDatabases(const ConnectionConfig &config) {
    QNetworkRequest req = createBaseRequest(config, "databases", APPCOMM_ADMIN);
    QNetworkReply *reply = m_network->get(req);
    connect(reply, &QNetworkReply::finished, this, &Server::onResponseFinished);
}

void Server::createCollection(const ConnectionConfig &config,
                              const QString &name,
                              const QJsonArray &permissions) {
    QString path = QString("databases/%1/collections").arg(config.dbId);
    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_ADMIN);
    QJsonObject body;
    body["collectionId"] = config.collectionId;
    body["name"] = name;
    if (!permissions.isEmpty()) {
        body["permissions"] = permissions;
    } else {
        body["permissions"] = QJsonArray{"read(\"any\")", "create(\"any\")"};
    }
    QNetworkReply *reply = m_network->post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, &Server::onResponseFinished);
}

void Server::deleteCollection(const ConnectionConfig &config) {
    QString path = QString("databases/%1/collections/%2")
                       .arg(config.dbId, config.collectionId);
    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_ADMIN);
    QNetworkReply *reply = m_network->deleteResource(req);
    connect(reply, &QNetworkReply::finished, this, &Server::onResponseFinished);
}

void Server::updateCollectionPermissions(const ConnectionConfig &config,
                                         const QJsonArray &permissions) {
    QString path = QString("databases/%1/collections/%2")
                       .arg(config.dbId, config.collectionId);
    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_ADMIN);
    QJsonObject body;
    body["permissions"] = permissions;
    QNetworkReply *reply = m_network->sendCustomRequest(req, "PATCH", QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, &Server::onResponseFinished);
}

void Server::createAttribute(const ConnectionConfig &config,
                             const QString &type,
                             const QString &key,
                             bool required,
                             const QJsonObject &options) {
    QString path = QString("databases/%1/collections/%2/attributes/%3")
                       .arg(config.dbId, config.collectionId, type);
    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_ADMIN);
    QJsonObject body;
    body["key"] = key;
    body["required"] = required;
    for (auto it = options.begin(); it != options.end(); ++it) {
        body[it.key()] = it.value();
    }
    QNetworkReply *reply = m_network->post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, &Server::onResponseFinished);
}

void Server::createIndex(const ConnectionConfig &config,
                        const QString &key,
                        const QString &type,
                        const QStringList &attributes) {
    QString path = QString("databases/%1/collections/%2/indexes")
                       .arg(config.dbId, config.collectionId);
    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_ADMIN);
    QJsonArray attrs;
    for (const QString &attr : attributes) {
        attrs.append(attr);
    }
    QJsonObject body;
    body["key"] = key;
    body["type"] = type;
    body["attributes"] = attrs;
    QNetworkReply *reply = m_network->post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, &Server::onResponseFinished);
}

void Server::createUser(const ConnectionConfig &config,
                       const QString &email,
                       const QString &password,
                       const QString &name) {
    QNetworkRequest req = createBaseRequest(config, "users", APPCOMM_ADMIN);
    QJsonObject body;
    body["userId"] = "unique()";
    body["email"] = email;
    body["password"] = password;
    if (!name.isEmpty()) {
        body["name"] = name;
    }
    QNetworkReply *reply = m_network->post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, &Server::onResponseFinished);
}

void Server::deleteUser(const ConnectionConfig &config, const QString &userId) {
    QString path = QString("users/%1").arg(userId);
    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_ADMIN);
    QNetworkReply *reply = m_network->deleteResource(req);
    connect(reply, &QNetworkReply::finished, this, &Server::onResponseFinished);
}

void Server::listUsers(const ConnectionConfig &config, const QJsonArray &queries) {
    QNetworkRequest req = createBaseRequest(config, "users", APPCOMM_ADMIN);
    QUrl url = req.url();
    if (!queries.isEmpty()) {
        QUrlQuery query;
        for (const QJsonValue &q : queries) {
            appendListQueryItem(q.toString(), &query, true);
        }
        url.setQuery(query);
    }
    req.setUrl(url);
    QNetworkReply *reply = m_network->get(req);
    connect(reply, &QNetworkReply::finished, this, &Server::onResponseFinished);
}

void Server::listCollections(const ConnectionConfig &config) {
    QString path = QString("databases/%1/collections").arg(config.dbId);
    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_ADMIN);
    QNetworkReply *reply = m_network->get(req);
    connect(reply, &QNetworkReply::finished, this, &Server::onResponseFinished);
}

void Server::listDocuments(const ConnectionConfig &config, const QJsonArray &queries) {
    QString path = QString("databases/%1/collections/%2/documents")
    .arg(config.dbId)
        .arg(config.collectionId);

    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_ADMIN);
    QUrl url = req.url();

    if (!queries.isEmpty()) {
        QStringList encodedQueries;

        for (const QJsonValue &q : queries) {
            const QString queryString = q.toString().trimmed();
            if (!queryString.isEmpty()) {
                encodedQueries.append(
                    "queries[]=" + QUrl::toPercentEncoding(queryString)
                    );
            }
        }

        url.setQuery(encodedQueries.join("&"), QUrl::TolerantMode);
    }

    req.setUrl(url);

    qDebug() << "[Server] listDocuments URL:" << url.toString();

    QNetworkReply *reply = m_network->get(req);
    connect(reply, &QNetworkReply::finished, this, &Server::onResponseFinished);
}

void Server::createDocument(const ConnectionConfig &config, const QJsonObject &data) {
    QString path = QString("databases/%1/collections/%2/documents")
                       .arg(config.dbId, config.collectionId);
    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_ADMIN);
    QJsonObject body;
    body["documentId"] = "unique()";
    body["data"] = data;
    body["permissions"] = QJsonArray{"read(\"any\")", "write(\"any\")"};
    QNetworkReply *reply = m_network->post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, &Server::onResponseFinished);
}

void Server::deleteDocument(const ConnectionConfig &config, const QString &documentId) {
    QString path = QString("/databases/%1/collections/%2/documents/%3")
                       .arg(config.dbId)
                       .arg(config.collectionId)
                       .arg(documentId);
    QNetworkRequest req = createBaseRequest(config, path, APPCOMM_ADMIN);
    QNetworkReply *reply = m_network->deleteResource(req);
    connect(reply, &QNetworkReply::finished, this, &Server::onResponseFinished);
}
