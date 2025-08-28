// localmessagemanager.cpp
#include "localmessagemanager.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QRandomGenerator>
#include <QDebug>
#include <QCoreApplication>
LocalMessageManager::LocalMessageManager(QObject *parent) : QObject(parent)
{
    ensureDirExists(); // 初始化时确保目录存在
}

bool LocalMessageManager::ensureDirExists()
{
    // 消息文件存储路径：程序运行目录/chat_messages/
    QString dirPath = QCoreApplication::applicationDirPath() + "/chat_messages/";
    QDir dir(dirPath);
    if (!dir.exists()) {
        return dir.mkpath("."); // 创建目录（包括父目录）
    }
    return true;
}


QString LocalMessageManager::getConversationFilePath(const QString& myId, const QString& friendId)
{
    // 按ID字典序排序，避免"myId-friendId"和"friendId-myId"生成两个文件
    QString sortedIds = (myId < friendId) ? myId + "_" + friendId : friendId + "_" + myId;
    return QCoreApplication::applicationDirPath() + "/chat_messages/" + sortedIds + ".json";
}

QString LocalMessageManager::generateMsgId()
{
    // 生成唯一ID：时间戳（毫秒）+ 3位随机数，确保不重复
    qint64 timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    int random = QRandomGenerator::global()->bounded(1000);
    return QString("%1%2").arg(timestamp).arg(random, 3, 10, QLatin1Char('0'));
}


bool LocalMessageManager::saveMessage(const QJsonObject& message)
{
    // 从消息中提取收发双方ID
    QString fromId = message["fromUserId"].toString();
    QString toId = message["toUserId"].toString();
    if (fromId.isEmpty() || toId.isEmpty()) {
        qWarning() << "消息缺少收发方ID，保存失败";
        return false;
    }

    QString filePath = getConversationFilePath(fromId, toId);
    QFile file(filePath);
    QJsonArray msgArray;

    // 若文件已存在，先读取已有消息
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "无法打开消息文件（读）：" << file.errorString();
            return false;
        }
        // 解析已有JSON数组
        QByteArray data = file.readAll();
        file.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isArray()) {
            msgArray = doc.array();
        }
    }

    // 添加新消息到数组
    msgArray.append(message);

    // 写入文件（覆盖原文件）
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "无法打开消息文件（写）：" << file.errorString();
        return false;
    }
    QJsonDocument doc(msgArray);
    file.write(doc.toJson(QJsonDocument::Indented)); // 格式化JSON，便于调试
    file.close();
    return true;
}

QList<QJsonObject> LocalMessageManager::loadMessages(const QString& myId, const QString& friendId, const QString& lastMsgId)
{
    QList<QJsonObject> messages;
    QString filePath = getConversationFilePath(myId, friendId);
    QFile file(filePath);

    if (!file.exists()) {
        return messages; // 无历史消息，返回空列表
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "加载消息失败：" << file.errorString();
        return messages;
    }

    // 解析文件中的JSON数组
    QByteArray data = file.readAll();
    file.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isArray()) {
        qWarning() << "消息文件格式错误，无法解析";
        return messages;
    }

    // 过滤出增量消息（ID大于lastMsgId）
    QJsonArray msgArray = doc.array();
    foreach (const QJsonValue& val, msgArray) {
        if (val.isObject()) {
            QJsonObject msg = val.toObject();
            QString msgId = msg["id"].toString();
            // 比较消息ID（基于时间戳，可直接字符串比较）
            if (msgId > lastMsgId) {
                messages.append(msg);
            }
        }
    }

    return messages;
}
