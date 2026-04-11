#ifndef MESSAGEQUERYSERVICE_H
#define MESSAGEQUERYSERVICE_H

#include <QObject>
#include <QJsonArray>

namespace appcomm {

namespace server {

class MessageQueryService : public QObject {
    Q_OBJECT
public:
    explicit MessageQueryService(QObject *parent = nullptr);

    QJsonArray channelMessages(const QString &channelId, int limit) const;
    QJsonArray channelDocuments(const QString &channelId, int limit = -1) const;
    QJsonArray channelMemberDocuments(const QString &channelId,
                                      const QString &userId,
                                      int limit = 1) const;
    QJsonArray messageDocuments(const QString &messageId, int limit = 1) const;

    static QString equalQuery(const QString &key, const QString &value);

signals:

private:
    static QString escapeQueryValue(const QString &value);
};

} // namespace server

} // namespace appcomm

#endif // MESSAGEQUERYSERVICE_H
