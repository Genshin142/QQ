#include "SmoothMessageWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <QFile>
#include <QGraphicsDropShadowEffect>

SmoothMessageWidget::SmoothMessageWidget(const QString &userId, const QString &userName, const QString &content,
                                       const QString &timeStr, Side side, bool isGroup, bool isImage, QWidget *parent)
    : QWidget(parent), m_userId(userId), m_userName(userName), m_content(content),
      m_time(timeStr), m_side(side), m_isGroup(isGroup), m_isImage(isImage)
{
    setupUi();
}

void SmoothMessageWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 5, 10, 5);
    mainLayout->setSpacing(2);

    // 1. 时间显示 (在此演示中，由于界面有滚动区域，时间可以由外部统一添加作为 Separator，
    // 或者就在 Widget 顶部显示。这里选择在顶部显示一个居中的小时间)
    // QString displayTime = formatTime(m_time);
    // ... 可以根据需要添加 ...

    // 2. 核心布局（头像 + [名字 + 气泡]）
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(10);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    // 头像
    m_avatarLabel = new QLabel();
    m_avatarLabel->setFixedSize(40, 40);
    m_avatarLabel->setPixmap(getRoundAvatar(m_userId));
    m_avatarLabel->setStyleSheet("background: transparent;");

    // 气泡和名字的容器
    QWidget *bubbleContainer = new QWidget();
    QVBoxLayout *vLayout = new QVBoxLayout(bubbleContainer);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(2);

    // 发送者姓名 (仅在群聊或其他人的消息时显示)
    if (m_side == Left || m_isGroup) {
        m_nameLabel = new QLabel(m_userName);
        m_nameLabel->setStyleSheet("color: #666; font-size: 10px; margin-left: 5px; margin-right: 5px;");
        if (m_side == Right) {
            m_nameLabel->setAlignment(Qt::AlignRight);
        } else {
            m_nameLabel->setAlignment(Qt::AlignLeft);
        }
        vLayout->addWidget(m_nameLabel);
    }

    // 消息气泡
    m_bubbleWidget = new MessageBubblePart(m_content, m_side == Right, m_isImage, this);
    if (m_side == Right) {
        vLayout->addWidget(m_bubbleWidget, 0, Qt::AlignRight);
    } else {
        vLayout->addWidget(m_bubbleWidget, 0, Qt::AlignLeft);
    }

    if (m_side == Right) {
        contentLayout->addStretch();
        contentLayout->addWidget(bubbleContainer);
        contentLayout->addWidget(m_avatarLabel, 0, Qt::AlignTop);
    } else {
        contentLayout->addWidget(m_avatarLabel, 0, Qt::AlignTop);
        contentLayout->addWidget(bubbleContainer);
        contentLayout->addStretch();
    }

    mainLayout->addLayout(contentLayout);
    setLayout(mainLayout);
}

QPixmap SmoothMessageWidget::getRoundAvatar(const QString &userId)
{
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
    return roundAvatar;
}

void SmoothMessageWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
}

bool SmoothMessageWidget::eventFilter(QObject *watched, QEvent *event)
{
    return QWidget::eventFilter(watched, event);
}

QString SmoothMessageWidget::formatTime(const QString &timeStr)
{
    // 这里简单复用原有的逻辑或简化
    return timeStr;
}

// ---------------------------------------------------------
// MessageBubblePart 实现
// ---------------------------------------------------------

MessageBubblePart::MessageBubblePart(const QString &text, bool isRight, bool isImage, QWidget *parent)
    : QWidget(parent), m_text(text), m_isRight(isRight), m_isImage(isImage)
{
    int maxWidth = 400; // 最大宽度
    int padding = 12;

    if (m_isImage) {
        QByteArray ba = QByteArray::fromBase64(m_text.toUtf8());
        if (m_image.loadFromData(ba)) {
            m_image = m_image.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            setFixedSize(m_image.width() + 2 * padding, m_image.height() + 2 * padding);
        } else {
            setFixedSize(100, 30);
        }
    } else {
        QFont font("Microsoft YaHei", 10);
        setFont(font);
        QFontMetrics fm(font);
        QRect rect = fm.boundingRect(0, 0, maxWidth - 2 * padding, 1000, Qt::TextWordWrap, m_text);
        
        int w = rect.width() + 2 * padding + 10;
        int h = rect.height() + 2 * padding;
        setFixedSize(w, h);
    }
}

void MessageBubblePart::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect bubbleRect = rect().adjusted(2, 2, -2, -2);
    QPainterPath path;
    int radius = 12;
    path.addRoundedRect(bubbleRect, radius, radius);

    if (m_isRight) {
        // 右边：渐变蓝
        QLinearGradient gradient(bubbleRect.topLeft(), bubbleRect.bottomRight());
        gradient.setColorAt(0, QColor(0, 145, 255));
        gradient.setColorAt(1, QColor(0, 100, 255));
        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen);
        painter.drawPath(path);

        painter.setPen(Qt::white);
    } else {
        // 左边：白色/浅灰
        painter.setBrush(QColor(240, 240, 240));
        painter.setPen(Qt::NoPen);
        painter.drawPath(path);

        painter.setPen(QColor(33, 33, 33));
    }

    int padding = 12;
    if (m_isImage) {
        if (!m_image.isNull()) {
            painter.drawPixmap(padding, padding, m_image);
        } else {
            painter.drawText(bubbleRect, Qt::AlignCenter, "[图片加载失败]");
        }
    } else {
        painter.drawText(bubbleRect.adjusted(padding, padding, -padding, -padding), 
                         Qt::TextWordWrap, m_text);
    }
}
