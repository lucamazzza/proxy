#ifndef MEMBERSHIPSERVICE_H
#define MEMBERSHIPSERVICE_H

#include <QObject>

namespace appcomm {

namespace server {


class MembershipService : public QObject {
    Q_OBJECT
public:
    explicit MembershipService(QObject *parent = nullptr);

signals:
};

} // namespace server

} // namespace appcomm

#endif // MEMBERSHIPSERVICE_H
