#include "recentmessagecache.h"

using namespace appcomm;

RecentMessageCache::RecentMessageCache(int capacity, QObject *parent)
    : QObject{parent}
    , m_capacity(capacity)
{}

void RecentMessageCache::addMessage(const QString &messageId, const QJsonObject &data) {
    if (messageId.isEmpty()) return;
    if (m_capacity <= 0) return;
    
    if (m_cache.contains(messageId)) {
        m_cache[messageId].data = data;
        m_cache[messageId].timestamp = QDateTime::currentMSecsSinceEpoch();
        return;
    }
    if (m_queue.size() >= m_capacity) {
        QString oldest = m_queue.dequeue();
        m_cache.remove(oldest);
    }
    CachedMessage msg;
    msg.messageId = messageId;
    msg.data = data;
    msg.timestamp = QDateTime::currentMSecsSinceEpoch();
    m_queue.enqueue(messageId);
    m_cache.insert(messageId, msg);
}

QList<CachedMessage> RecentMessageCache::getMessagesSince(const QString &messageId) const {
    QList<CachedMessage> result;
    if (!m_cache.contains(messageId)) return result;
    bool found = false;
    for (const auto &id : m_queue) {
        if (found) {
            result.append(m_cache.value(id));
        } else if (id == messageId) {
            found = true;
        }
    }
    return result;
}

CachedMessage RecentMessageCache::getMessage(const QString &messageId) const {
    return m_cache.value(messageId);
}

bool RecentMessageCache::contains(const QString &messageId) const {
    return m_cache.contains(messageId);
}

QList<CachedMessage> RecentMessageCache::getAllMessages() const {
    QList<CachedMessage> result;
    for (const auto &id : m_queue) {
        result.append(m_cache.value(id));
    }
    return result;
}

void RecentMessageCache::clear() {
    m_queue.clear();
    m_cache.clear();
}