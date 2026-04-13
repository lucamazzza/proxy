#ifndef MESSAGELISTMODEL_H
#define MESSAGELISTMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>

struct ChatMessageItem
{
    QString sender;
    QString body;
    QString timestamp;
    bool system = false; //distingue messaggio utente da messaggio sistema
};

class MessageListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum MessageRoles {
        SenderRole = Qt::UserRole + 1,
        BodyRole,
        TimestampRole,
        SystemRole
    };

    explicit MessageListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addMessage(const ChatMessageItem &message);
    void clear();

private:
    QVector<ChatMessageItem> m_messages;
};

#endif // MESSAGELISTMODEL_H