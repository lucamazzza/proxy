#include "messagelistmodel.h"

MessageListModel::MessageListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int MessageListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_messages.size();
}

QVariant MessageListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.size()) {
        return {};
    }

    const ChatMessageItem &message = m_messages.at(index.row());

    switch (role) {
    case SenderRole:
        return message.sender;
    case BodyRole:
        return message.body;
    case TimestampRole:
        return message.timestamp;
    case SystemRole:
        return message.system;
    default:
        return {};
    }
}

QHash<int, QByteArray> MessageListModel::roleNames() const
{
    return {
        {SenderRole, "sender"},
        {BodyRole, "body"},
        {TimestampRole, "timestamp"},
        {SystemRole, "system"}
    };
}

void MessageListModel::addMessage(const ChatMessageItem &message)
{
    const int row = m_messages.size();
    beginInsertRows(QModelIndex(), row, row);
    m_messages.append(message);
    endInsertRows();
}

void MessageListModel::clear()
{
    beginResetModel();
    m_messages.clear();
    endResetModel();
}