// localmessagemanager.h
#ifndef LOCALMESSAGEMANAGER_H
#define LOCALMESSAGEMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>

class LocalMessageManager : public QObject
{
    Q_OBJECT
public:
    explicit LocalMessageManager(QObject *parent = nullptr);

    // 保存单条消息到本地文件
    bool saveMessage(const QJsonObject& message);

    // 加载与指定好友的消息（支持增量拉取）
    QList<QJsonObject> loadMessages(const QString& myId, const QString& friendId, const QString& lastMsgId = "0");

    // 生成唯一消息ID（基于时间戳+随机数）
    QString generateMsgId();


private:
    // 获取对话文件路径（以两个用户ID组合命名，确保唯一）
    QString getConversationFilePath(const QString& myId, const QString& friendId);

    // 确保本地消息目录存在（如不存在则创建）
    bool ensureDirExists();


};

#endif // LOCALMESSAGEMANAGER_H
