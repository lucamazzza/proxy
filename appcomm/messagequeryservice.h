#ifndef MESSAGEQUERYSERVICE_H
#define MESSAGEQUERYSERVICE_H

#include <QObject>

class MessageQueryService : public QObject {
    Q_OBJECT
public:
    explicit MessageQueryService(QObject *parent = nullptr);

signals:
};

#endif // MESSAGEQUERYSERVICE_H
