/*!
 * @file ratelimiter.h
 * @brief Token bucket rate limiter for controlling request throughput
 *
 * Implements a token bucket algorithm to limit the rate of requests
 * to the Appwrite backend, preventing API quota exhaustion and ensuring
 * fair resource usage.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#ifndef RATELIMITER_H
#define RATELIMITER_H

#include <QObject>
#include <QDateTime>

namespace appcomm {

namespace client {

/*!
 * @brief Interface for request rate limiting.
 *
 * Allows AppcommClient to use fake rate limiters during unit tests.
 */
class IRateLimiter
{
public:
    virtual ~IRateLimiter() = default;

    virtual bool allowRequest() = 0;
    virtual void reset() = 0;
};

/*!
 * @brief Token bucket rate limiter.
 *
 * Controls request throughput using a token bucket algorithm.
 */
class RateLimiter : public QObject, public IRateLimiter
{
    Q_OBJECT

public:
    /*!
     * @brief Constructs a RateLimiter instance.
     *
     * @param maxTokens Maximum bucket capacity (burst size).
     * @param refillRate Tokens added per second.
     * @param parent Parent QObject for memory management.
     */
    explicit RateLimiter(int maxTokens, int refillRate, QObject *parent = nullptr);

    /*!
     * @brief Checks if a request can be processed.
     *
     * Attempts to consume one token from the bucket. Refills token based
     * on elapsed time before checking availability.
     *
     * @return true if request is allowed, false if limit exceeded.
     */
    bool allowRequest() override;

    /*!
     * @brief Resets the rate limiter state.
     *
     * Restores available tokens to maximum capacity and resets
     * the last refill timestamp to current time.
     */
    void reset() override;

    /*!
     * @brief Gets the current available tokens.
     *
     * @return Number of tokens currently available.
     */
    inline int availableTokens() const { return m_availableTokens; }

    /*!
     * @brief Gets the maximum token capacity.
     *
     * @return Maximum number of tokens.
     */
    inline int maxTokens() const { return m_maxTokens; }

    /*!
     * @brief Gets the refill rate of the bucket.
     *
     * @return Number of tokens added each second.
     */
    inline int refillRate() const { return m_refillRate; }

private:

    /*!
     * @brief Refills the bucket with the maximum token capacity.
     *
     * Calculates how many tokens should be added based on the time
     * elapsed since the last refill, capped at maximum capacity.
     */
    void refillTokens();

    int m_maxTokens;        ///< Maximum bucket capacity.
    int m_availableTokens;  ///< Current available tokens.
    int m_refillRate;       ///< Tokens added each second.
    QDateTime m_lastRefill; ///< Last refill Timestamp.
};

} // namespace client

} // namespace appcomm

#endif // RATELIMITER_H
