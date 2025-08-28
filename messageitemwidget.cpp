#include "messageitemwidget.h"
#include <QPainterPath>
MessageItemWidget::MessageItemWidget(bool isMyMessage,
                                     const QString& timeStr,
                                     const QString& content,
                                     const QString& fromUserId,
                                     bool isGroupChat,
                                     const QString& userName,
                                     QWidget* parent)
    : QWidget(parent) {
    // 主布局（垂直方向）
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 5, 0, 5);
    mainLayout->setSpacing(3);

    // 添加时间标签（如果时间字符串不为空）
    if (!timeStr.isEmpty()) {
        QString displayTime = formatTime(timeStr);
        QLabel* timeLabel = new QLabel(displayTime);
        timeLabel->setStyleSheet("QLabel { color: #888; font-size: 12px; text-align: center; }");
        timeLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(timeLabel, 0, Qt::AlignHCenter);
    }

    // 内容布局（水平方向）
    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(8);

    // 头像容器
    QWidget* avatarContainer = new QWidget();
    avatarContainer->setFixedSize(40, 40);

    // 头像标签
    QLabel* avatarLabel = new QLabel(avatarContainer);
    avatarLabel->setGeometry(0, 0, 40, 40);
    avatarLabel->setStyleSheet("border-radius: 20px; background-color: #eee;");

    // 加载头像（优先本地，其次默认）
    QString avatarPath = QString("./local_avatars/%1.png").arg(fromUserId);
    QPixmap avatar(avatarPath);
    if (avatar.isNull()) {
        avatar.load(":/images/QQ.png");
        if (avatar.isNull()) {
            // 加载失败时使用灰色占位图
            avatar = QPixmap(40, 40);
            avatar.fill(Qt::lightGray);
        }
    }

    // 处理圆形头像
    avatar = avatar.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap roundAvatar(40, 40);
    roundAvatar.fill(Qt::transparent);
    QPainter painter(&roundAvatar);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(0, 0, 40, 40);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, avatar);
    avatarLabel->setPixmap(roundAvatar);

    // 消息内容标签
    QLabel* contentLabel = new QLabel(content);
    QString styleSheet = QString("QLabel {") +
                         "   background-color: %1;"
                         "   color: #333; "
                         "   padding: 8px 12px; "
                         "   border-radius: 15px; "
                         "   white-space: wrap; "
                         "%2"
                         "}";
    // 根据消息类型设置样式
    styleSheet = styleSheet.arg(isMyMessage ? "#d4f4d4" : "#f0f0f0")
                     .arg((isGroupChat && !isMyMessage) ? "   margin-top: 5px; " : "");
    contentLabel->setStyleSheet(styleSheet);
    contentLabel->setWordWrap(true);
    contentLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    // 用户名标签
    QLabel* nameLabel = new QLabel(userName);
    nameLabel->setStyleSheet(
        "QLabel {"
        "   color: #666666; "
        "   font-size: 10px; "
        "   padding: 0 2px; "
        "   background-color: rgba(255, 255, 255, 80); "
        "   border-radius: 2px; "
        "}"
        );
    nameLabel->setAlignment(Qt::AlignCenter);

    // 根据是否为自己的消息设置布局方向
    if (isMyMessage) {
        // 自己的消息：内容居右，头像在右侧
        contentLayout->addStretch();
        contentLayout->addWidget(contentLabel);
        contentLayout->addWidget(avatarContainer);
        // 用户名居右
        mainLayout->addWidget(nameLabel, 0, Qt::AlignRight | Qt::AlignTop);
    } else {
        // 他人的消息：头像在左侧，内容在右侧
        contentLayout->addWidget(avatarContainer);
        contentLayout->addWidget(contentLabel);
        contentLayout->addStretch();
        // 用户名居左
        mainLayout->addWidget(nameLabel, 0, Qt::AlignLeft | Qt::AlignTop);
    }

    mainLayout->addLayout(contentLayout);
    setLayout(mainLayout);
}

QSize MessageItemWidget::sizeHint() const {
    QSize hint = QWidget::sizeHint();
    if (parentWidget()) {
        // 限制最大宽度为父窗口的70%
        int maxWidth = parentWidget()->width() * 0.7;
        return QSize(qMin(hint.width(), maxWidth), hint.height());
    }
    return hint;
}

QString MessageItemWidget::formatTime(const QString& timeStr) {
    QDateTime msgTime = QDateTime::fromString(timeStr, "yyyy-MM-dd hh:mm:ss");
    if (!msgTime.isValid()) {
        msgTime = QDateTime::fromString(timeStr, "yyyy-MM-dd HH:mm:ss");
    }

    if (!msgTime.isValid()) {
        return timeStr;
    }

    QDateTime now = QDateTime::currentDateTime();
    QDate msgDate = msgTime.date();
    QDate today = now.date();
    QDate yesterday = today.addDays(-1);

    // 判断时间段（凌晨/上午/下午/晚上）
    int hour = msgTime.time().hour();
    QString period;
    if (hour >= 5 && hour < 12) {
        period = "上午";
    } else if (hour >= 12 && hour < 18) {
        period = "下午";
    } else if (hour >= 18 && hour < 24) {
        period = "晚上";
    } else {
        period = "凌晨";
    }

    // 格式化显示
    if (msgDate == today) {
        return QString("%1%2").arg(period).arg(msgTime.toString("hh:mm"));
    } else if (msgDate == yesterday) {
        return QString("昨天 %1%2").arg(period).arg(msgTime.toString("hh:mm"));
    } else {
        return QString("%1 %2%3")
        .arg(msgTime.toString("MM/dd"))
            .arg(period)
            .arg(msgTime.toString("hh:mm"));
    }
}
