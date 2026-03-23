#ifndef APPCOMMSERVER_H
#define APPCOMMSERVER_H

#include <QObject>

namespace appcomm {

namespace server {

class AppcommServer : public QObject {
    Q_OBJECT
public:
    explicit AppcommServer(QObject *parent = nullptr);

signals:
};

} // namespace server

} // namespace appcomm

#endif // APPCOMMSERVER_H
