#include "ratelimiter.h"

using namespace appcomm;

RateLimiter::RateLimiter(int maxTokens, int refillRate, QObject *parent)
    : QObject{parent}
    , m_maxTokens(maxTokens)
    , m_availableTokens(maxTokens)
    , m_refillRate(refillRate)
    , m_lastRefill(QDateTime::currentDateTime())
{}

bool RateLimiter::allowRequest() {
    refillTokens();
    if (m_availableTokens > 0) {
        m_availableTokens--;
        return true;
    }
    return false;
}

void RateLimiter::reset() {
    m_availableTokens = m_maxTokens;
    m_lastRefill = QDateTime::currentDateTime();
}

void RateLimiter::refillTokens() {
    QDateTime now = QDateTime::currentDateTime();
    qint64 elapsedMs = m_lastRefill.msecsTo(now);
    if (elapsedMs <= 0) return;
    double elapsedSeconds = elapsedMs / 1000.0;
    int tokensToAdd = static_cast<int>(elapsedSeconds * m_refillRate);
    if (tokensToAdd > 0) {
        m_availableTokens = std::min(m_availableTokens + tokensToAdd, m_maxTokens);
        m_lastRefill = now;
    }
}
