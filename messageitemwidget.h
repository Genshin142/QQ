#ifndef MESSAGEITEMWIDGET_H
#define MESSAGEITEMWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QDateTime>

class MessageItemWidget : public QWidget {
    Q_OBJECT
public:
    /*
     * 构造函数，创建消息项控件实例
     * 参数说明：
     * isMyMessage - 是否为当前用户发送的消息（true表示自己发送）
     * timeStr - 消息发送时间字符串，格式应为"yyyy-MM-dd hh:mm:ss"
     * content - 消息的文本内容
     * fromUserId - 发送者的用户ID，用于加载对应的头像
     * isGroupChat - 是否为群聊消息（true表示群聊）
     * userName - 发送者的用户名，群聊时显示
     */
    MessageItemWidget(bool isMyMessage,
                      const QString& timeStr,
                      const QString& content,
                      const QString& fromUserId,
                      bool isGroupChat,
                      const QString& userName,
                      QWidget* parent = nullptr);

    QSize sizeHint() const override;

private:
    //格式化时间字符串
    QString formatTime(const QString& timeStr);
};

#endif // MESSAGEITEMWIDGET_H
