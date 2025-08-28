#include "ChatDelegate.h"
#include <QPainterPath>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFontMetrics>
#include <QLinearGradient>

ChatItemDelegate::ChatItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent), m_avatarCache(100), m_imageCache(100)
{
}

void ChatItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid()) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    bool isHeader = index.data(IsHeaderRole).toBool();
    if (isHeader) {
        // Draw time separator
        QString timeStr = index.data(TimeRole).toString();
        QRect textRect = option.rect;
        painter->setPen(QColor(153, 153, 153));
        QFont font = painter->font();
        font.setPointSize(9);
        painter->setFont(font);
        painter->drawText(textRect, Qt::AlignCenter, timeStr);
        painter->restore();
        return;
    }

    bool isMyMessage = index.data(IsMyMessageRole).toBool();
    bool isImage = index.data(IsImageRole).toBool();
    bool isGroup = index.data(IsGroupChatRole).toBool();
    QString content = index.data(ContentRole).toString();
    QString userName = index.data(UserNameRole).toString();
    QString userId = index.data(FromUserIdRole).toString();

    int padding = 12;
    int avatarSize = 40;
    int avatarMargin = 10;
    int bubbleMargin = 50; // Distance from opposite side

    QRect rect = option.rect;
    
    // Draw Avatar
    QRect avatarRect;
    if (isMyMessage) {
        avatarRect = QRect(rect.right() - avatarSize - avatarMargin, rect.top() + 5, avatarSize, avatarSize);
    } else {
        avatarRect = QRect(rect.left() + avatarMargin, rect.top() + 5, avatarSize, avatarSize);
    }
    painter->drawPixmap(avatarRect, getRoundAvatar(userId));

    // Draw Name (if needed)
    int topOffset = 5;
    if (!isMyMessage || isGroup) {
        painter->setPen(QColor(102, 102, 102));
        QFont nameFont = painter->font();
        nameFont.setPointSize(8);
        painter->setFont(nameFont);
        QRect nameRect;
        if (isMyMessage) {
            nameRect = QRect(rect.left(), rect.top() + topOffset, rect.width() - avatarSize - 2 * avatarMargin - 5, 15);
            painter->drawText(nameRect, Qt::AlignRight, userName);
        } else {
            nameRect = QRect(rect.left() + avatarSize + 2 * avatarMargin + 5, rect.top() + topOffset, rect.width(), 15);
            painter->drawText(nameRect, Qt::AlignLeft, userName);
        }
        topOffset += 18;
    }

    // Calculate Bubble Rect
    int maxWidth = rect.width() - avatarSize - 2 * avatarMargin - bubbleMargin;
    QRect bubbleRect;
    QPixmap img;
    if (isImage) {
        if (m_imageCache.contains(content)) {
            img = *m_imageCache[content];
        } else {
            QByteArray ba = QByteArray::fromBase64(content.toUtf8());
            img.loadFromData(ba);
            img = img.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_imageCache.insert(content, new QPixmap(img));
        }
        bubbleRect.setSize(QSize(img.width() + 2 * padding, img.height() + 2 * padding));
    } else {
        QFont contentFont("Microsoft YaHei", 10);
        painter->setFont(contentFont);
        QFontMetrics fm(contentFont);
        QRect textRect = fm.boundingRect(0, 0, maxWidth - 2 * padding, 1000, Qt::TextWordWrap, content);
        bubbleRect.setSize(QSize(textRect.width() + 2 * padding + 10, textRect.height() + 2 * padding));
    }

    if (isMyMessage) {
        bubbleRect.moveTopRight(QPoint(rect.right() - avatarSize - 2 * avatarMargin, rect.top() + topOffset));
    } else {
        bubbleRect.moveTopLeft(QPoint(rect.left() + avatarSize + 2 * avatarMargin, rect.top() + topOffset));
    }

    // Paint Bubble
    QPainterPath path;
    path.addRoundedRect(bubbleRect.adjusted(2, 2, -2, -2), 12, 12);
    if (isMyMessage) {
        QLinearGradient gradient(bubbleRect.topLeft(), bubbleRect.bottomRight());
        gradient.setColorAt(0, QColor(0, 145, 255));
        gradient.setColorAt(1, QColor(0, 100, 255));
        painter->setBrush(gradient);
        painter->setPen(Qt::NoPen);
        painter->drawPath(path);
        painter->setPen(Qt::white);
    } else {
        painter->setBrush(QColor(240, 240, 240));
        painter->setPen(Qt::NoPen);
        painter->drawPath(path);
        painter->setPen(QColor(33, 33, 33));
    }

    // Paint Content
    if (isImage) {
        if (!img.isNull()) {
            painter->drawPixmap(bubbleRect.left() + padding, bubbleRect.top() + padding, img);
        }
    } else {
        painter->drawText(bubbleRect.adjusted(padding, padding, -padding, -padding), Qt::TextWordWrap, content);
    }

    painter->restore();
}

QSize ChatItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid()) return QSize();

    if (index.data(IsHeaderRole).toBool()) {
        return QSize(option.rect.width(), 40);
    }

    bool isMyMessage = index.data(IsMyMessageRole).toBool();
    bool isGroup = index.data(IsGroupChatRole).toBool();
    bool isImage = index.data(IsImageRole).toBool();
    QString content = index.data(ContentRole).toString();

    int padding = 12;
    int avatarSize = 40;
    int avatarMargin = 10;
    int bubbleMargin = 50;

    int topOffset = 5;
    if (!isMyMessage || isGroup) {
        topOffset += 18;
    }

    int height = 0;
    if (isImage) {
        QPixmap img;
        if (m_imageCache.contains(content)) {
            img = *m_imageCache[content];
        } else {
            QByteArray ba = QByteArray::fromBase64(content.toUtf8());
            img.loadFromData(ba);
            img = img.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_imageCache.insert(content, new QPixmap(img));
        }
        height = img.height() + 2 * padding + topOffset + 10;
    } else {
        int maxWidth = option.rect.width() - avatarSize - 2 * avatarMargin - bubbleMargin;
        if (maxWidth <= 0) maxWidth = 400; 
        QFont contentFont("Microsoft YaHei", 10);
        QFontMetrics fm(contentFont);
        QRect textRect = fm.boundingRect(0, 0, maxWidth - 2 * padding, 1000, Qt::TextWordWrap, content);
        height = textRect.height() + 2 * padding + topOffset + 10;
    }

    return QSize(option.rect.width(), qMax(height, avatarSize + 15));
}

QPixmap ChatItemDelegate::getRoundAvatar(const QString &userId) const
{
    if (m_avatarCache.contains(userId)) return *m_avatarCache[userId];

    QString avatarPath = QString("./local_avatars/%1.png").arg(userId);
    QPixmap avatar(avatarPath);
    if (avatar.isNull()) {
        avatar.load(":/images/QQ.png");
        if (avatar.isNull()) {
            avatar = QPixmap(40, 40);
            avatar.fill(Qt::lightGray);
        }
    }

    avatar = avatar.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap roundAvatar(40, 40);
    roundAvatar.fill(Qt::transparent);
    QPainter painter(&roundAvatar);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(0, 0, 40, 40);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, avatar);
    
    m_avatarCache.insert(userId, new QPixmap(roundAvatar));
    return roundAvatar;
}
