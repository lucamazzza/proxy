#ifndef ERRORMODEL_H
#define ERRORMODEL_H

#include <QString>

namespace errormodel {

enum class ErrorCode {
    None = 0,
    AuthError,
    NetworkError,
    ParseError,
    ProtocolError,
    PermissionError,
    TimeoutError,
    RateLimitExceeded,
    Unknown
};

struct AppCommError {
    ErrorCode code;
    QString message;
    int httpStatus;

    bool hasError();
    QString toString();
};

}

#endif // ERRORMODEL_H
