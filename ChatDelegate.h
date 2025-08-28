#ifndef CHATDELEGATE_H
#define CHATDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QDateTime>
#include <QPixmap>
#include <QCache>

class ChatItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    enum MessageRole {
        FromUserIdRole = Qt::UserRole + 1,
        UserNameRole,
        ContentRole,
        TimeRole,
        IsMyMessageRole,
        IsGroupChatRole,
        IsImageRole,
        IsHeaderRole  // For time separators
    };

    explicit ChatItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QPixmap getRoundAvatar(const QString &userId) const;
    mutable QCache<QString, QPixmap> m_avatarCache; // Cache for performance
    mutable QCache<QString, QPixmap> m_imageCache;  // Cache for message images
};

#endif // CHATDELEGATE_H
