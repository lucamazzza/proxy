#include "garbagecollector.h"

using namespace appcomm;
using namespace appcomm::server;

GarbageCollector::GarbageCollector(appwritesdk::Server *server,
                                   const appwritesdk::ConnectionConfig &config,
                                   QObject *parent)
    : QObject{parent}
    , m_server(server)
    , m_config(config)
{
    connect(m_server, &appwritesdk::Server::requestSuccess, this, &GarbageCollector::onDocumentsListed);
    connect(m_server, &appwritesdk::Server::requestError, this, &GarbageCollector::onError);
}

void GarbageCollector::runCleanup(const model::PersistencePolicy &policy) {
    if (policy.messageTTL <= 0) {
        emit cleanupComplete(0);
        return;
    }
    QDateTime cutoffDate = QDateTime::currentDateTimeUtc().addDays(-policy.messageTTL);
    QJsonArray queries;
    queries.append(QString("lessThan(\"timestamp\", \"%1\")").arg(cutoffDate.toString(Qt::ISODate)));
    queries.append("limit(100");
    m_deletedCount = 0;
    m_docsToDelete.clear();
    m_server->listDocuments(m_config, queries);
}

void GarbageCollector::onDocumentsListed(const QJsonObject &data) {
    QJsonArray documents = data.value("documents").toArray();
    if (documents.isEmpty()) {
        emit cleanupComplete(m_deletedCount);
        return;
    }
    disconnect(m_server, &appwritesdk::Server::requestSuccess, this, &GarbageCollector::onDocumentsListed);
    connect(m_server, &appwritesdk::Server::requestSuccess, this, &GarbageCollector::onDocumentDeleted);
    for (const QJsonValue &doc : std::as_const(documents)) {
        QString docId = doc.toObject().value("$id").toString();
        if (!docId.isEmpty()) {
            m_docsToDelete.append(docId);
        }
    }
    if (!m_docsToDelete.isEmpty()) {
        QString docId = m_docsToDelete.takeFirst();
        m_server->deleteDocument(m_config, docId);
    }
}

void GarbageCollector::onDocumentDeleted(const QJsonObject &data) {
    Q_UNUSED(data);
    m_deletedCount++;
    if (!m_docsToDelete.isEmpty()) {
        QString docId = m_docsToDelete.takeFirst();
        m_server->deleteDocument(m_config, docId);
    } else {
        disconnect(m_server, &appwritesdk::Server::requestSuccess, this, &GarbageCollector::onDocumentDeleted);
        connect(m_server, &appwritesdk::Server::requestSuccess, this, &GarbageCollector::onDocumentsListed);
        emit cleanupComplete(m_deletedCount);
    }
}

void GarbageCollector::onError(int code, const QString &message) {
    emit cleanupError(QString("Cleanup failed (code %1): %2").arg(code).arg(message));
}