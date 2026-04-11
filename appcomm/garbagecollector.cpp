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
    Q_ASSERT(server != nullptr);
    connect(m_server, &appwritesdk::Server::requestSuccess, this, &GarbageCollector::onDocumentsListed);
    connect(m_server, &appwritesdk::Server::requestError, this, &GarbageCollector::onError);
}

void GarbageCollector::runCleanup(const model::PersistencePolicy &policy) {
    if (policy.messageTTL <= 0) {
        m_cleanupInProgress = false;
        m_cutoffTimestamp.clear();
        m_docsToDelete.clear();
        m_deletedCount = 0;
        emit cleanupComplete(0);
        return;
    }
    const QDateTime cutoffDate = QDateTime::currentDateTimeUtc().addDays(-policy.messageTTL);
    m_cutoffTimestamp = cutoffDate.toString(Qt::ISODate);
    m_cleanupInProgress = true;
    m_deletedCount = 0;
    m_docsToDelete.clear();
    m_server->listDocuments(m_config, cleanupQueries());
}

void GarbageCollector::onDocumentsListed(const QJsonObject &data) {
    if (!m_cleanupInProgress) {
        return;
    }
    QJsonArray documents = data.value("documents").toArray();
    if (documents.isEmpty()) {
        m_cleanupInProgress = false;
        m_cutoffTimestamp.clear();
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
    if (!m_cleanupInProgress) {
        return;
    }
    Q_UNUSED(data);
    m_deletedCount++;
    if (!m_docsToDelete.isEmpty()) {
        QString docId = m_docsToDelete.takeFirst();
        m_server->deleteDocument(m_config, docId);
    } else {
        disconnect(m_server, &appwritesdk::Server::requestSuccess, this, &GarbageCollector::onDocumentDeleted);
        connect(m_server, &appwritesdk::Server::requestSuccess, this, &GarbageCollector::onDocumentsListed);
        m_server->listDocuments(m_config, cleanupQueries());
    }
}

void GarbageCollector::onError(int code, const QString &message) {
    m_cleanupInProgress = false;
    m_cutoffTimestamp.clear();
    m_docsToDelete.clear();
    emit cleanupError(QString("Cleanup failed (code %1): %2").arg(code).arg(message));
}

QJsonArray GarbageCollector::cleanupQueries() const {
    QJsonArray queries;
    queries.append(QString("lessThan(\"timestamp\", \"%1\")").arg(m_cutoffTimestamp));
    queries.append(QString("limit(%1)").arg(CLEANUP_BATCH_SIZE));
    return queries;
}
