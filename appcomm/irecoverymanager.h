#ifndef IRECOVERYMANAGER_H
#define IRECOVERYMANAGER_H

#include <QString>

namespace appcomm {

namespace client {

class IRecoveryManager {
public:
    virtual ~IRecoveryManager() = default;
    virtual void requestFrom(const QString& messageId) = 0;
};

} // namespace client

} // namespace appcomm

#endif // IRECOVERYMANAGER_H
