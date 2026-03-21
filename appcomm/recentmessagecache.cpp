#include "recentmessagecache.h"

using namespace appcomm;

RecentMessageCache::RecentMessageCache(int capacity, QObject *parent)
    : QObject{parent}
    , m_capacity(capacity)
{}

void RecentMessageCache::addMessage(const model::Message &msg) {
    if (!msg.isValid()) return;
    if (m_capacity <= 0) return;
    if (m_cache.contains(msg.messageId)) {
        m_cache[msg.messageId] = msg;
        return;
    }
    if (m_queue.size() >= m_capacity) {
        QString oldest = m_queue.dequeue();
        m_cache.remove(oldest);
    }
    m_queue.enqueue(msg.messageId);
    m_cache.insert(msg.messageId, msg);
}

QList<model::Message> RecentMessageCache::getMessagesSince(const QString &messageId) const {
    QList<model::Message> result;
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

model::Message RecentMessageCache::getMessage(const QString &messageId) const {
    return m_cache.value(messageId);
}

bool RecentMessageCache::contains(const QString &messageId) const {
    return m_cache.contains(messageId);
}

QList<model::Message> RecentMessageCache::getAllMessages() const {
    QList<model::Message> result;
    for (const auto &id : m_queue) {
        result.append(m_cache.value(id));
    }
    return result;
}

void RecentMessageCache::clear() {
    m_queue.clear();
    m_cache.clear();
}