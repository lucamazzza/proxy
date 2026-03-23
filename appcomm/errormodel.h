/*!
 * @file errormodel.h
 * @brief Error handling model for the AppComm communication layer.
 *
 * Defines standardized error codes and error structures used to represent
 * failures across frontend, backend, and proxy components.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#ifndef ERRORMODEL_H
#define ERRORMODEL_H

#include <QString>

/*!
 * @brief Namespace containing error-related types for AppComm.
 *
 * Provides error codes, error objects, and utility functions used
 * to report and format failures in the communication layer.
 */
namespace errormodel {

/*!
 * @brief Enumeration of possible error types in the system.
 *
 * Provides a unified classification of errors that may occur during
 * communication, authentication, parsing, or system operations.
 */
enum class ErrorCode {
    None = 0,          ///< No error
    AuthError,         ///< Authentication failure
    NetworkError,      ///< Network-related error
    ParseError,        ///< JSON or data parsing error
    ProtocolError,     ///< Invalid or unexpected protocol behavior
    PermissionError,   ///< Access denied or insufficient permissions
    TimeoutError,      ///< Request timeout
    RateLimitExceeded, ///< Too many requests (rate limiting)
    Unknown            ///< Unknown or unspecified error
};

/*!
 * @brief Converts an error code to its string representation.
 *
 * @param code Error code to convert
 * @return Human-readable name of the error code
 */
QString errorCodeToString(ErrorCode code);

/*!
 * @brief Represents an error in the AppComm system.
 *
 * Encapsulates the error classification, a descriptive message,
 * and an optional HTTP status code when the failure originates
 * from an HTTP-based operation.
 */
struct AppCommError {
    ErrorCode code = ErrorCode::None;   ///< Error classification
    QString message;                    ///< Human-readable error message
    int httpStatus = 0;                 ///< HTTP status code (if applicable)

    /*!
     * @brief Checks whether this object represents an error.
     * @return True if the error code is not None
     */
    bool hasError() const;

    /*!
     * @brief Converts the error to a readable string.
     * @return Formatted string representation of the error
     */
    QString toString() const;
};

} // namespace errormodel

#endif // ERRORMODEL_H
