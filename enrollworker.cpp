#include <QJsonObject>
#include <QJsonDocument>
#include "networkmanager.h"
#include <QDir>
#include <QCoreApplication>
#include "enrollworker.h"
// 日志工具函数：输出到控制台并写入日志文件
// 参数：message-日志内容，type-日志类型（INFO/ERROR等）
static void writeLog(const QString& message, const QString& type = "INFO") {
    // 构建根目录下的"日志"文件夹路径
    QString logDirPath = QCoreApplication::applicationDirPath() + "/日志";
    // 检查并创建"日志"文件夹（不存在则创建）
    QDir logDir(logDirPath);
    if (!logDir.exists()) {
        if (!logDir.mkpath(".")) { // 创建文件夹（包括必要的父目录）
            qWarning() << "无法创建日志文件夹：" << logDirPath;
            return; // 创建失败则直接返回，避免后续错误
        }
    }

    // 获取当前日期并格式化为yyyyMMdd形式
    QString dateStr = QDateTime::currentDateTime().toString("yyyyMMdd");
    // 构建带日期的日志文件名，存储到日志文件夹下
    QString fileName = logDirPath + QString("/注册子线程日志_%1.log").arg(dateStr);

    // 格式化日志信息（包含时间戳）
    QString logMsg = QString("[%1] %2: %3")
                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                         .arg(type)
                         .arg(message);

    // 输出到控制台
    qDebug() << logMsg;

    // 写入日志文件（追加模式）
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << logMsg << "\n";
        file.close();
    } else {
        qWarning() << "无法打开日志文件：" << fileName << "，错误：" << file.errorString();
    }
}

EnrollWorker::EnrollWorker(const QString& phone, const QString& name, const QString& password, QObject* parent)
    : QObject(parent)
    , m_phoneNumber(phone)
    , m_name(name)
    , m_password(password) {
    writeLog(QString("创建注册工作对象，手机号: %1，昵称: %2").arg(phone).arg(name));
}

// 执行注册逻辑：构建请求并发送
void EnrollWorker::doEnroll() {
    writeLog(QString("发起注册请求 (NetworkManager)"));

    connect(&NetworkManager::instance(), &NetworkManager::registerResponse, this, [this](bool success, const QString& message) {
        if (success) {
            writeLog("注册成功");
            emit enrollResult(true, "账号注册成功！");
        } else {
            writeLog(QString("注册失败: %1").arg(message), "ERROR");
            emit enrollResult(false, message);
        }
        emit finished();
    });

    connect(&NetworkManager::instance(), &NetworkManager::errorOccurred, this, [this](const QString& error) {
        writeLog("网络错误: " + error, "ERROR");
        emit errorOccurred(error);
        emit finished();
    });

    QJsonObject json;
    json["type"] = "register";
    json["phone"] = m_phoneNumber;
    json["name"] = m_name;
    json["password"] = m_password;
    NetworkManager::instance().sendPacket(json);
}
