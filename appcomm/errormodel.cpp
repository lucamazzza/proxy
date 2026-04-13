/*!
 * @file errormodel.cpp
 * @brief Implementation of AppComm error model helpers.
 *
 * @copyright Copyright (c) 2026 SUPSI
 */

#include "errormodel.h"

using namespace appcomm::errormodel;

bool AppCommError::hasError() const
{
    return code != ErrorCode::None;
}

namespace appcomm {
namespace errormodel {

QString errorCodeToString(ErrorCode code)
{
    switch (code) {
    case ErrorCode::None: return "None";
    case ErrorCode::AuthError: return "AuthError";
    case ErrorCode::NetworkError: return "NetworkError";
    case ErrorCode::ParseError: return "ParseError";
    case ErrorCode::ProtocolError: return "ProtocolError";
    case ErrorCode::PermissionError: return "PermissionError";
    case ErrorCode::TimeoutError: return "TimeoutError";
    case ErrorCode::RateLimitExceeded: return "RateLimitExceeded";
    case ErrorCode::Unknown: return "Unknown";
    }
    return "Unknown";
}

}
}

QString AppCommError::toString() const
{
    return QString("Error[%1] (%2): %3")
    .arg(errorCodeToString(code))
        .arg(httpStatus)
        .arg(message);
}
