#include "widget.h"
#include "ui_widget.h"
#include <QTabBar>
#include <QPainter>
#include <QPixmap>
#include <QPainterPath>
#include <QFileDialog> // 添加文件对话框头文件
#include "logoutworker.h"
#include <QThread>
#include <QTimer>
#include "usermanager.h"
#include <QCloseEvent>
#include <QDir>
#include <QJsonArray>
#include<QUrlQuery>
#include <QVBoxLayout>
#include <QScrollBar>
#include "messageitemwidget.h"
const int AVATAR_SIZE = 40;
// 统一的好友列表样式（树控件和列表控件共用）
const QString FRIEND_LIST_STYLE = R"(
    QTreeWidget, QListView {
        background-color: white;
        outline: none;
        border: none;
    }
    QTreeWidget::item, QListView::item {
        height: 60px; /* 项高随头像增大而增加（50头像+10边距） */
        border-bottom: 1px solid #f0f0f0;
        background-color: white;
        color: #333333;
        padding-left: 15px; /* 增加左内边距，避免头像贴边 */
    }
    /* 关键：设置树控件图标的固定显示尺寸 */
    QTreeWidget::item::icon {
        width: 50px;
        height: 50px;
    }
    QListView::item::icon {
        width: 50px;
        height: 50px;
        image-rendering: smooth; /* 强制平滑渲染 */
    }
    QTreeWidget::item:selected, QListView::item:selected {
        background-color: #ccebff;
        color: #000000;
        border: none;
    }
    QTreeWidget::item:hover:!selected, QListView::item:hover:!selected {
        background-color: rgba(200, 200, 200, 80);
    }
    QTreeWidget::item:has-children {
        font-weight: bold;
        color: #0099ff;
    }
)";
// 静态成员初始化
QMutex Widget::logMutex;
QString Widget::logFileName = "";
// 日志工具函数实现
void Widget::writeLog(const QString& message, LogType type) {
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
    // 生成带日期的日志文件名（每天一个日志文件），存储到日志文件夹下
    QString dateStr = QDateTime::currentDateTime().toString("yyyyMMdd");
    QString fileName = logDirPath + QString("/主窗口日志_%1.log").arg(dateStr);

    // 日志时间戳（精确到毫秒）
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    // 日志类型字符串
    QString typeStr;
    switch (type) {
    case LOG_INFO:    typeStr = "[INFO]"; break;
    case LOG_WARNING: typeStr = "[WARNING]"; break;
    case LOG_ERROR:   typeStr = "[ERROR]"; break;
    }

    // 构造日志内容
    QString logStr = QString("%1 %2 Widget: %3").arg(timeStr).arg(typeStr).arg(message);
    // 输出到控制台
    qDebug() << logStr;

    // 写入日志文件（加锁保证线程安全）
    QMutexLocker locker(&logMutex);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << logStr << "\n";
        file.close();
    } else {
        qWarning() << "无法打开日志文件:" << fileName << "，错误：" << file.errorString();
    }
}

QString Widget::getUserNameById(const QString &userId)
{
    writeLog(QString("开始根据用户ID %1 获取用户名").arg(userId), LOG_INFO);
    if (m_userIdToName.contains(userId)) {
        writeLog(QString("找到用户ID %1 对应的用户名: %2").arg(userId).arg(m_userIdToName[userId]), LOG_INFO);
        return m_userIdToName[userId];
    }
    // 未找到时返回ID（避免显示空），可根据需求优化
    writeLog(QString("未找到用户ID %1 对应的用户名，返回ID").arg(userId), LOG_WARNING);
    return userId;
}


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);  // 加载UI界面布局，初始化所有UI控件
    writeLog("Widget主窗口初始化开始", LOG_INFO);  // 记录初始化开始日志

    init_background_color();  // 初始化窗口背景渐变动画
    writeLog("背景渐变动画初始化完成", LOG_INFO);  // 记录背景动画初始化完成

    init_tabwidget();  // 初始化主标签页控件（隐藏标签栏等样式设置）
    init_groupbox();   // 初始化主分组框样式（设置边框、背景色等）
    init_label_title();  // 初始化标题标签（设置图标、文字样式）

    // 初始化功能按钮（联系人、聊天、好友、群组）
    init_pushButton_contact_person();  // 初始化联系人按钮
    init_pushButton_chat();            // 初始化聊天按钮

    init_tabWidget_contact_person();  // 初始化联系人标签页（隐藏标签栏等）

    initListWidgetRecord();             // 初始化聊天记录文本框（设置只读等属性）

    init_label_name();


    writeLog("Widget主窗口初始化完成", LOG_INFO);  // 记录主窗口初始化完成

    // 初始化网络管理器，用于处理HTTP请求
    m_networkManager = new QNetworkAccessManager(this);
    writeLog("网络管理器初始化完成", LOG_INFO);
    // 初始化定时器，设置较短的基础间隔（如1秒）
    QTimer* msgTimer = new QTimer(this);
    msgTimer->setInterval(1000); // 1秒基础间隔
    writeLog("消息拉取定时器创建，初始间隔1秒", LOG_INFO);

    // 动态调整请求频率的逻辑
    connect(msgTimer, &QTimer::timeout, this, [=]() {
        static int idleCount = 0; // 连续空闲计数器
        static const int MAX_IDLE_COUNT = 5; // 最大空闲次数（5秒）

        // 如果当前有聊天对象，才发起请求
        if (!m_currentFriendId.isEmpty()) {
            fetchNewMessages();

            // 检查是否有新消息
            if (hasNewMessages) { // 需要在代码中维护hasNewMessages标志
                idleCount = 0; // 有新消息，重置计数器
                msgTimer->setInterval(1000); // 保持1秒高频
            } else {
                idleCount++;
                // 连续无新消息时，逐渐降低频率（最长5秒）
                if (idleCount >= MAX_IDLE_COUNT) {
                    msgTimer->setInterval(5000);
                }
            }
        } else {
            // 无聊天对象时降低频率
            msgTimer->setInterval(5000);
        }
    });

    msgTimer->start();
    writeLog("消息拉取定时器启动", LOG_INFO);

    fetchAllAvatars();
    initFriendList();                 // 初始化好友列表（添加默认好友项、设置头像）
    // 初始化好友列表后连接点击信号
    initTreeWidgetFriend();
    connect(ui->listView_friend, &QListView::clicked,
            this, &Widget::onFriendListViewItemClicked);
    writeLog("好友列表点击信号连接完成", LOG_INFO);

    // 连接发送按钮信号
    connect(ui->pushButton_send, &QPushButton::clicked,
            this, &Widget::on_pushButton_send_clicked);
    writeLog("发送按钮点击信号连接完成", LOG_INFO);


    // 只在初始化时获取一次用户列表，避免定时器重复调用
    getAllUsers();
    writeLog("开始获取所有用户信息", LOG_INFO);
    init_label_avatar();  // 初始化头像标签（设置圆形样式、加载头像）
    ui->textEdit_send->installEventFilter(this);
    writeLog("为textEdit_send安装事件过滤器完成", LOG_INFO);






}

Widget::~Widget()
{
    writeLog("Widget主窗口开始销毁", LOG_INFO);  // 新增日志
    delete ui;
    writeLog("Widget主窗口销毁完成", LOG_INFO);  // 新增日志
}
void Widget::fetchAllAvatars() {
    writeLog("开始获取所有用户头像信息", LOG_INFO);

    QUrl url("http://101.37.68.162:8080/api/get_allAvatar");
    QNetworkRequest request(url);

    QNetworkReply* reply = m_networkManager->get(request);
    writeLog("已发送获取所有用户头像的GET请求", LOG_INFO);

    // 设置超时处理（10秒）
    QTimer::singleShot(10000, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
            qWarning() << "获取所有头像请求超时";
        }
    });

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            writeLog(QString("收到所有用户头像响应，数据大小: %1 字节").arg(responseData.size()), LOG_INFO);

            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
            if (doc.isNull()) {
                writeLog(QString("解析所有用户头像响应失败: %1（偏移量: %2）")
                             .arg(parseError.errorString())
                             .arg(parseError.offset), LOG_ERROR);
                reply->deleteLater();
                return;
            }

            QJsonObject response = doc.object();
            if (response["success"].toBool()) {
                QJsonArray avatars = response["avatars"].toArray();
                writeLog(QString("成功解析所有用户头像信息，共 %1 个用户").arg(avatars.size()), LOG_INFO);

                // 创建本地保存目录
                QDir avatarDir("local_avatars");
                if (!avatarDir.exists()) {
                    if (avatarDir.mkpath(".")) {
                        writeLog("成功创建本地头像保存目录: local_avatars", LOG_INFO);
                    } else {
                        writeLog("创建本地头像保存目录失败", LOG_ERROR);
                        reply->deleteLater();
                        return;
                    }
                }

                // 获取当前登录用户ID
                QString currentUserId = UserManager::getInstance().getUserId();
                writeLog(QString("当前登录用户ID: %1，准备保存其头像路径").arg(currentUserId), LOG_INFO);

                // 遍历所有头像信息并下载保存
                foreach (const QJsonValue& val, avatars) {
                    QJsonObject avatarObj = val.toObject();
                    QString userId = avatarObj["user_id"].toString();
                    QString avatarUrl = avatarObj["avatar_url"].toString();

                    if (userId.isEmpty() || avatarUrl.isEmpty()) {
                        writeLog("用户头像信息不完整，跳过", LOG_WARNING);
                        continue;
                    }

                    // 构建完整的头像URL
                    QUrl fullUrl;
                    if (avatarUrl.startsWith("http")) {
                        fullUrl = QUrl(avatarUrl);
                    } else {
                        fullUrl = QUrl("http://101.37.68.162:8080" + avatarUrl);
                    }

                    // 下载并保存单个头像
                    QNetworkRequest avatarRequest(fullUrl);
                    QNetworkReply* avatarReply = m_networkManager->get(avatarRequest);

                    // 捕获当前迭代的userId（lambda中需用拷贝值）
                    const QString currentIterUserId = userId;

                    connect(avatarReply, &QNetworkReply::finished, this, [=]() {
                        if (avatarReply->error() == QNetworkReply::NoError) {
                            QByteArray imageData = avatarReply->readAll();
                            QString savePath = QString("local_avatars/%1.png").arg(currentIterUserId);

                            QFile file(savePath);
                            if (file.open(QIODevice::WriteOnly)) {
                                file.write(imageData);
                                file.close();
                                writeLog(QString("成功保存用户 %1 的头像到 %2").arg(currentIterUserId).arg(savePath), LOG_INFO);

                                // 关键：如果是当前用户的头像，写入avatar.txt
                                if (currentIterUserId == currentUserId) {
                                    QFile avatarFile("avatar.txt");
                                    if (avatarFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                                        QTextStream out(&avatarFile);
                                        out << savePath; // 写入当前用户头像的本地路径
                                        avatarFile.close();
                                        writeLog(QString("已将当前用户头像路径保存到 avatar.txt: %1").arg(savePath), LOG_INFO);
                                    } else {
                                        writeLog(QString("无法打开 avatar.txt 保存当前用户头像路径: %1")
                                                     .arg(avatarFile.errorString()), LOG_ERROR);
                                    }
                                }
                            } else {
                                writeLog(QString("无法打开文件保存用户 %1 的头像: %2")
                                             .arg(currentIterUserId)
                                             .arg(file.errorString()), LOG_ERROR);
                            }
                        } else {
                            writeLog(QString("下载用户 %1 的头像失败: %2")
                                         .arg(currentIterUserId)
                                         .arg(avatarReply->errorString()), LOG_ERROR);
                        }
                        avatarReply->deleteLater();
                    });
                }
            } else {
                writeLog("服务器返回获取头像失败", LOG_ERROR);
            }
        } else {
            writeLog(QString("获取所有用户头像网络错误（错误码: %1）: %2")
                         .arg(reply->error())
                         .arg(reply->errorString()), LOG_ERROR);
        }
        reply->deleteLater();
        writeLog("所有用户头像获取请求处理完成", LOG_INFO);
    });
}
// 初始化好友列表TreeWidget
void Widget::initTreeWidgetFriend() {
    writeLog("开始初始化好友列表TreeWidget", LOG_INFO);
    ui->treeWidget_friend->setHeaderHidden(true);
    ui->treeWidget_friend->setIndentation(20); // 增加缩进，适配更大头像

    // 创建"我的好友"根节点
    QTreeWidgetItem *rootItem = new QTreeWidgetItem();
    rootItem->setText(0, "我的好友");
    ui->treeWidget_friend->addTopLevelItem(rootItem);

    // 先断开已有连接，避免重复关联
    disconnect(ui->treeWidget_friend, &QTreeWidget::itemDoubleClicked,
               this, &Widget::onTreeWidgetFriendItemDoubleClicked);

    // 再重新关联
    connect(ui->treeWidget_friend, &QTreeWidget::itemDoubleClicked,
            this, &Widget::onTreeWidgetFriendItemDoubleClicked);

    // 初始化listView_friend模型
    m_chatListModel = new QStandardItemModel(this);

    ui->listView_friend->setModel(m_chatListModel);

    // 应用统一样式
    ui->treeWidget_friend->setStyleSheet(FRIEND_LIST_STYLE);
    ui->listView_friend->setStyleSheet(FRIEND_LIST_STYLE);

    // 显式设置树控件图标大小（与头像尺寸一致）
    ui->treeWidget_friend->setIconSize(QSize(AVATAR_SIZE, AVATAR_SIZE));

    // 显式设置 ListView 图标大小和样式
    ui->listView_friend->setIconSize(QSize(AVATAR_SIZE, AVATAR_SIZE));
    ui->listView_friend->setStyleSheet(FRIEND_LIST_STYLE);
    writeLog("设置ListView图标大小和样式完成", LOG_INFO);

    // 强制刷新样式（确保新设置生效）
    ui->listView_friend->style()->unpolish(ui->listView_friend);
    ui->listView_friend->style()->polish(ui->listView_friend);
    ui->listView_friend->update();
    writeLog("初始化好友列表TreeWidget完成", LOG_INFO);
}

void Widget::init_label_name()
{
    writeLog("开始初始化用户名称标签", LOG_INFO);
    QString username = UserManager::getInstance().getName();
    if (username.isEmpty()) {
        writeLog("获取当前登录用户名称失败，使用默认名称", LOG_WARNING);
        username = "未命名用户";
    }
    ui->label_name->setText(username);

    // 设置字体为微软雅黑
    QFont font("微软雅黑", 12);
    ui->label_name->setFont(font);

    // 强制设置文字为纯白色，背景完全透明
    ui->label_name->setStyleSheet(
        "QLabel {"
        "    color: rgb(255, 255, 255);"
        "    background-color: rgba(0, 0, 0, 0);"
        "    border: none;"
        "    padding: 2px;"
        "}"
        );

    // 关键修改：设置文本居中对齐（水平和垂直都居中）
    ui->label_name->setAlignment(Qt::AlignCenter);  // 替换原有对齐方式

    // 强制刷新样式
    ui->label_name->style()->unpolish(ui->label_name);
    ui->label_name->style()->polish(ui->label_name);
    ui->label_name->update();
    writeLog("初始化用户名称标签完成", LOG_INFO);
}



// 从服务器获取所有用户信息
void Widget::getAllUsers() {
    writeLog("开始获取所有用户信息", LOG_INFO);
    QUrl url("http://101.37.68.162:8080/api/get_all_users");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->get(request);
    writeLog("发送获取所有用户信息的GET请求", LOG_INFO);

    // 使用lambda表达式作为中间层传递参数
    connect(reply, &QNetworkReply::finished, this, [=]() {
        writeLog("获取所有用户信息请求完成，开始处理响应", LOG_INFO);
        onGetAllUsersFinished(reply);
    });
    // 确保reply释放
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

}
// 处理ListView好友项点击
void Widget::onFriendListViewItemClicked(const QModelIndex &index) {
    writeLog("好友ListView项点击事件触发", LOG_INFO);


    if (!index.isValid())
    {
        writeLog("无效的列表项索引，忽略点击事件", LOG_WARNING);
        return;
    }

    QStandardItem *item = m_chatListModel->itemFromIndex(index);
    if (!item)
    {
        writeLog("未找到对应的列表项，忽略点击事件", LOG_WARNING);
        return;
    }
    writeLog(QString("当前好友列表数量: %1").arg(m_chatListModel->rowCount()), LOG_INFO);

    // 获取好友信息
    m_currentFriendId = item->data(Qt::UserRole).toString();
    m_currentFriendName = item->text();
    QString friendPhone = item->data(Qt::UserRole + 1).toString();

    writeLog(QString("开始与 %1 (ID: %2) 通信").arg(m_currentFriendName).arg(m_currentFriendId), LOG_INFO);

    // 清空聊天框
    writeLog("清空聊天记录和发送框", LOG_INFO);
    ui->listWidget_recieve->clear();
    ui->textEdit_send->clear();

    // 关键修改：切换好友时清空本地消息ID集合
    writeLog("切换好友，清空本地消息ID集合", LOG_INFO);
    m_localMessageIds.clear();

    writeLog("重置当前好友的最后消息ID为0", LOG_INFO);
    m_lastMessageIds[m_currentFriendId] = "0";


    // 立即拉取该好友的所有历史消息（因lastMessageId已重置为0）
    writeLog("开始拉取当前好友的历史消息", LOG_INFO);
    fetchNewMessages();


    // 更新窗口标题
    setWindowTitle(QString("与 %1 聊天中").arg(m_currentFriendName));
}

// 发送消息处理
void Widget::on_pushButton_send_clicked() {


    static bool isSending = false; // 防止重复发送的标记
    QString message = ui->textEdit_send->toPlainText().trimmed();

    if (message.isEmpty()) {
        writeLog("发送消息为空，忽略", LOG_WARNING);
        return;
    }

    if (m_currentFriendId.isEmpty()) {
        writeLog("未选择聊天对象，无法发送消息", LOG_WARNING);
        return;
    }

    // 若正在发送中，直接返回
    if (isSending) {
        writeLog("消息正在发送中，请勿重复点击", LOG_WARNING);
        return;
    }

    // 获取当前用户信息
    QString myId = UserManager::getInstance().getUserId();
    QString myName = UserManager::getInstance().getName();

    if (myId.isEmpty()) {
        writeLog("当前用户未登录，无法发送消息", LOG_ERROR);
        return;
    }

    // 构建消息JSON
    QJsonObject msgJson;
    msgJson["fromUserId"] = myId;
    msgJson["toUserId"] = m_currentFriendId;
    msgJson["content"] = message;
    // 构建消息JSON时添加临时ID和时间戳（确保格式统一）
    QString tempMsgId = QUuid::createUuid().toString();
    QDateTime currentTime = QDateTime::currentDateTime();
    msgJson["tempId"] = tempMsgId;
    msgJson["time"] = currentTime.toString("yyyy-MM-dd hh:mm:ss");
    m_localMessageIds.insert(tempMsgId);
    if (m_currentFriendId == "0000") {
        msgJson["toUserId"] = "0000";  // 确保群聊消息的接收者是群聊ID
    }



    // 发送消息到服务器
    QUrl url("http://101.37.68.162:8080/api/send_message");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 标记为正在发送
    isSending = true;

    // 发送请求
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(msgJson).toJson());
    writeLog(QString("向 %1 发送消息: %2").arg(m_currentFriendId).arg(message), LOG_INFO);

    // 绑定响应处理
    connect(reply, &QNetworkReply::finished, this, [=]() {
        onSendMessageFinished();
        isSending = false; // 发送完成后重置标记
    });
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    // 错误处理
    connect(reply, &QNetworkReply::errorOccurred, this, [=](QNetworkReply::NetworkError error) {
        writeLog(QString("消息发送失败：%1").arg(reply->errorString()), LOG_ERROR);
        isSending = false; // 错误时也需重置标记
    });

    // 清空发送框
    ui->textEdit_send->clear();
}

void Widget::fetchNewMessages()
{
    if (!m_currentFriendId.isEmpty()) {
        QString myId = UserManager::getInstance().getUserId();
        if (myId.isEmpty()) {
            writeLog("用户未登录，无法拉取消息", LOG_WARNING);
            return;
        }

        QUrl url("http://101.37.68.162:8080/api/get_messages");
        QUrlQuery query;
        query.addQueryItem("toUserId", m_currentFriendId);
        // 添加当前用户ID和最后消息ID参数
        // 修改后（正确）：
        if (m_currentFriendId != "0000") {  // 仅单聊需要传递m_myUserId
            query.addQueryItem("m_myUserId", myId);
        }
        query.addQueryItem("m_lastMessageId", m_lastMessageIds.value(m_currentFriendId, "0"));
        url.setQuery(query);

        QNetworkRequest request(url);
        QNetworkReply* reply = m_networkManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [=]() {
            onMessagesFetched(reply);
            reply->deleteLater();
        });

        QTimer::singleShot(5000, reply, [reply]() {
            if (reply->isRunning()) {
                reply->abort();
                reply->deleteLater();
            }
        });
    }
}


void Widget::onMessagesFetched(QNetworkReply *reply)
{

    if (reply->error() != QNetworkReply::NoError) {
        writeLog(QString("拉取消息网络错误: %1（错误码：%2）")
                     .arg(reply->errorString())
                     .arg(reply->error()), LOG_ERROR);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();

    if (data.isEmpty()) {
        writeLog("收到空的消息数据，可能服务器无响应", LOG_WARNING);
        reply->deleteLater();
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull()) {
        writeLog(QString("解析消息失败：%1（偏移量：%2）")
                     .arg(parseError.errorString())
                     .arg(parseError.offset), LOG_ERROR);
        reply->deleteLater();
        return;
    }

    QJsonObject root = doc.object();
    if (!root["success"].toBool()) {
        writeLog("拉取消息失败: " + root["error"].toString(), LOG_ERROR);
        reply->deleteLater();
        return;
    }

    QJsonArray messages = root["messages"].toArray();
    QJsonArray uniqueMessages;
    QSet<QString> messageIds;

    for (const QJsonValue& val : messages) {
        QJsonObject msg = val.toObject();
        // 确保每条消息只被添加一次
        QString msgId = msg.contains("id") ? msg["id"].toString() : msg["tempId"].toString();
        if (!messageIds.contains(msgId) && !m_localMessageIds.contains(msgId)) {
            messageIds.insert(msgId);
            uniqueMessages.append(msg);
        }
    }

    if (!uniqueMessages.isEmpty()) {
        displayMessages(uniqueMessages);
        hasNewMessages = true; // 标记有新消息
        // 更新当前好友的最后消息ID
        if (!uniqueMessages.isEmpty()) {
            QString lastId = uniqueMessages.last().toObject()["id"].toString();
            if (!lastId.isEmpty()) {
                m_lastMessageIds[m_currentFriendId] = lastId;
            }
        }
    } else {
        hasNewMessages = false; // 标记无新消息
    }



    reply->deleteLater();
}

// 处理获取所有用户的响应（补充完整）
void Widget::onGetAllUsersFinished(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "获取用户列表失败：" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull()) {
        qDebug() << "解析用户列表失败";
        reply->deleteLater();
        return;
    }

    QJsonObject response = doc.object();
    if (response["success"].toBool()) {
        QJsonArray users = response["users"].toArray();
        // 遍历用户列表，构建ID-用户名映射
        for (const QJsonValue& userVal : users) {
            QJsonObject user = userVal.toObject();
            QString userId = user["user_id"].toString();
            QString userName = user["name"].toString();
            m_userIdToName[userId] = userName;  // 存储映射关系
        }

        QTreeWidgetItem *rootItem = ui->treeWidget_friend->topLevelItem(0);
        if (!rootItem) return;

        writeLog("清空treeWidget好友列表", LOG_INFO);
        rootItem->takeChildren();  // 清空现有好友项

        writeLog("清空listView好友列表", LOG_INFO);
        m_chatListModel->clear();  // 清空现有列表项

        // ======== 添加所有用户到列表 ========
        foreach (const QJsonValue &val, users) {
            QJsonObject user = val.toObject();
            QString userId = user["user_id"].toString();
            QString userName = user["name"].toString();
            QString userPhone = user["phone"].toString();

            // 添加到TreeWidget
            QTreeWidgetItem *userItem = new QTreeWidgetItem(rootItem);
            userItem->setText(0, userName);
            userItem->setData(0, Qt::UserRole, userId);
            userItem->setData(0, Qt::UserRole + 1, userPhone);

            // 添加到ListView
            QStandardItem *listItem = new QStandardItem();
            listItem->setText(userName);
            listItem->setData(userId, Qt::UserRole);
            listItem->setData(userPhone, Qt::UserRole + 1);

            // 加载用户头像（现有逻辑）
            QPixmap avatar;
            QString localAvatarPath = QString("local_avatars/%1.png").arg(userId);
            if (QFile::exists(localAvatarPath) && avatar.load(localAvatarPath)) {
                writeLog(QString("成功加载本地头像: %1").arg(localAvatarPath), LOG_INFO);
            } else {
                avatar.load(":/images/QQ.png");
                if (avatar.isNull()) {
                    avatar = QPixmap(AVATAR_SIZE, AVATAR_SIZE);
                    avatar.fill(Qt::lightGray);
                }
            }
            // 处理为圆形头像（现有逻辑）
            QPixmap roundAvatar(AVATAR_SIZE, AVATAR_SIZE);
            roundAvatar.fill(Qt::transparent);
            QPainter painter(&roundAvatar);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setClipRegion(QRegion(QRect(0, 0, AVATAR_SIZE, AVATAR_SIZE), QRegion::Ellipse));
            painter.drawPixmap(0, 0, avatar.scaled(AVATAR_SIZE, AVATAR_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));

            userItem->setIcon(0, QIcon(roundAvatar));
            listItem->setIcon(QIcon(roundAvatar));
            m_chatListModel->appendRow(listItem);
        }

        // ======== 新增：添加群聊项 ========
        // 1. 添加到TreeWidget
        QTreeWidgetItem *groupItem = new QTreeWidgetItem(rootItem);
        groupItem->setText(0, "所有人聊天群");  // 群聊名称
        groupItem->setData(0, Qt::UserRole, "0000");  // 群聊ID固定为0000
        groupItem->setData(0, Qt::UserRole + 1, "group");  // 标记为群聊类型

        // 2. 添加到ListView
        QStandardItem *groupListItem = new QStandardItem();
        groupListItem->setText("所有人聊天群");
        groupListItem->setData("0000", Qt::UserRole);  // 群聊ID
        groupListItem->setData("group", Qt::UserRole + 1);  // 标记为群聊

        // 3. 群聊头像处理（使用专用图标）
        QPixmap groupAvatar;
        QString groupAvatarPath = ":/images/group_icon.png";  // 建议添加群组图标资源
        if (!groupAvatar.load(groupAvatarPath)) {
            writeLog("群聊头像加载失败，使用默认灰色图标", LOG_WARNING);
            groupAvatar = QPixmap(AVATAR_SIZE, AVATAR_SIZE);
            groupAvatar.fill(Qt::lightGray);
        }
        // 处理为圆形头像
        QPixmap roundGroupAvatar(AVATAR_SIZE, AVATAR_SIZE);
        roundGroupAvatar.fill(Qt::transparent);
        QPainter groupPainter(&roundGroupAvatar);
        groupPainter.setRenderHint(QPainter::Antialiasing);
        groupPainter.setClipRegion(QRegion(QRect(0, 0, AVATAR_SIZE, AVATAR_SIZE), QRegion::Ellipse));
        groupPainter.drawPixmap(0, 0, groupAvatar.scaled(AVATAR_SIZE, AVATAR_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        groupItem->setIcon(0, QIcon(roundGroupAvatar));
        groupListItem->setIcon(QIcon(roundGroupAvatar));
        m_chatListModel->appendRow(groupListItem);

        rootItem->setExpanded(true);  // 展开根节点
    }

    reply->deleteLater();
}


// 处理TreeWidget双击事件
void Widget::onTreeWidgetFriendItemDoubleClicked(QTreeWidgetItem *item, int column) {
    QTreeWidgetItem *rootItem = ui->treeWidget_friend->topLevelItem(0);
    if (!rootItem) return;

    // 双击根节点：切换展开/折叠状态
    if (item == rootItem) {
        rootItem->setExpanded(!rootItem->isExpanded());
        return;
    }

    // 1. 同步树控件选中状态（确保只有当前项被选中）
    // 清除所有子节点的选中状态
    for (int i = 0; i < rootItem->childCount(); ++i) {
        rootItem->child(i)->setSelected(false);
    }
    // 设置当前项为选中状态（触发样式表中的选中样式）
    item->setSelected(true);

    // 2. 切换到第一个标签页
    ui->tabWidget->setCurrentIndex(0);

    // 3. 获取用户信息
    QString userName = item->text(0);
    QIcon userAvatar = item->icon(0);
    QString userId = item->data(0, Qt::UserRole).toString();
    QString userPhone = item->data(0, Qt::UserRole + 1).toString();

    // 4. 添加到listView_friend并保持样式一致
    QStandardItem *listItem = new QStandardItem();

    // 加载并处理圆形头像（与树控件保持一致）
    QPixmap avatar(":/images/QQ.png");
    if (avatar.isNull()) {
        writeLog("无法加载头像资源: images/QQ.png，使用默认灰色圆形头像", LOG_WARNING);
        // 创建灰色圆形占位图
        avatar = QPixmap(AVATAR_SIZE, AVATAR_SIZE);
        avatar.fill(Qt::transparent);
        QPainter painter(&avatar);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(Qt::lightGray);
        painter.drawEllipse(0, 0, AVATAR_SIZE, AVATAR_SIZE);
    } else {
        // 缩放原图并裁剪为圆形（与 treeWidget 保持完全一致的逻辑）
        QPixmap roundAvatar(AVATAR_SIZE, AVATAR_SIZE);
        roundAvatar.fill(Qt::transparent);
        QPainter painter(&roundAvatar);
        // 启用抗锯齿和高质量缩放（关键）
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        // 计算居中偏移（避免拉伸）
        QSize scaledSize = avatar.size().scaled(AVATAR_SIZE, AVATAR_SIZE, Qt::KeepAspectRatio);
        int xOffset = (AVATAR_SIZE - scaledSize.width()) / 2;
        int yOffset = (AVATAR_SIZE - scaledSize.height()) / 2;

        // 绘制圆形裁剪区域
        QPainterPath path;
        path.addEllipse(0, 0, AVATAR_SIZE, AVATAR_SIZE);
        painter.setClipPath(path);

        // 绘制缩放后的头像（使用高质量算法）
        painter.drawPixmap(
            xOffset, yOffset,  // 居中绘制
            avatar.scaled(
                scaledSize,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation  // 高质量缩放
                )
            );
        avatar = roundAvatar;  // 替换为圆形头像
    }

    // 显示头像和用户名（关键：设置图标尺寸与头像尺寸一致）
    listItem->setData(QVariant::fromValue(QIcon(avatar)), Qt::DecorationRole);
    listItem->setText(userName);
    // 存储额外用户信息
    listItem->setData(userId, Qt::UserRole);
    listItem->setData(userPhone, Qt::UserRole + 1);

    // 检查是否已存在该用户，避免重复添加
    bool exists = false;
    for (int i = 0; i < m_chatListModel->rowCount(); ++i) {
        QStandardItem *existingItem = m_chatListModel->item(i);
        if (existingItem->data(Qt::UserRole).toString() == userId) {
            exists = true;
            // 选中已存在的项
            ui->listView_friend->setCurrentIndex(existingItem->index());
            break;
        }
    }

    if (!exists) {
        m_chatListModel->appendRow(listItem);
        // 选中新添加的项
        ui->listView_friend->setCurrentIndex(listItem->index());
        ui->listView_friend->scrollToBottom(); // 滚动到最新项
    }

    // 5. 强制设置图标大小，确保与树控件一致
    ui->listView_friend->setIconSize(QSize(AVATAR_SIZE, AVATAR_SIZE));
    // 同步样式表（确保item高度适配头像）
    ui->listView_friend->setStyleSheet(FRIEND_LIST_STYLE);
}

void Widget::init_tabwidget()
{
    ui->tabWidget->tabBar()->hide();
}

void Widget::init_tabWidget_contact_person()
{
    ui->tabWidget_contact_person->tabBar()->hide();
}

void Widget::init_groupbox()
{
    ui->groupBox->setStyleSheet(
        "QGroupBox {"
        "    border: none;"
        "    background-color: #0099ff;"
        "}"
        );
}

void Widget::init_label_title()
{
    // 设置无边框并添加内边距，防止内容紧贴边缘
    ui->label_title->setStyleSheet(
        "QLabel {"
        "    border: none;"
        "    padding: 5px;  /* 内边距，让内容与边框保持距离 */"
        "}"
        );

    // 允许label自动调整大小以适应内容
    ui->label_title->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    // 确保label有足够的最小宽度来容纳图片和文字
    ui->label_title->setMinimumWidth(100);  // 根据实际图片大小调整

    // 设置内容：限制图片大小并调整布局
    // 添加width和height属性控制图片最大尺寸，避免图片过大
    ui->label_title->setText(
        QString("<img src=\":/images/QQ_label.png\" align=left width=24 height=24 style=\"margin-right:5px;\">") +
        QString("<span style=\"color:white; font-size:16pt; font-weight:normal;\">QQ</span>")
        );

    ui->label_title->setTextFormat(Qt::RichText);
    // 设置水平居中对齐（可选，根据整体布局调整）
    ui->label_title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    // 允许文本换行（如果内容过长）
    ui->label_title->setWordWrap(false);
}

// 初始化头像标签
void Widget::init_label_avatar()
{
    // 初始样式：圆形、白色背景（无灰色边框）
    ui->label_avatar->setStyleSheet(
        "QLabel {"
        "   border-radius: 30px;"
        "   background-color: white;"
        "}"
        );
    ui->label_avatar->setFixedSize(60, 60);
    ui->label_avatar->setFocusPolicy(Qt::NoFocus);

    // 安装事件过滤器，监听鼠标进入/离开事件
    ui->label_avatar->installEventFilter(this);

    onLoginSuccess();

}

// 初始化联系人按钮
void Widget::init_pushButton_contact_person()
{
    // 设置按钮无边框
    ui->pushButton_contact_person->setStyleSheet(
        "QPushButton {"
        "    border: none;"                  // 无边框
        "    background-color: transparent;" // 透明背景
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(224, 224, 224, 100);  /* 浅灰色，alpha=100（0-255） */"
        "    border-radius: 4px;"            // 悬浮时圆角效果
        "}"
        );

    // 设置图标并自适应按钮大小
    QIcon contactIcon(":/images/contactPerson.png");  // 替换为实际图标路径
    ui->pushButton_contact_person->setIcon(contactIcon);
    ui->pushButton_contact_person->setIconSize(QSize(24, 24));  // 设置初始图标大小

    // 允许按钮根据内容调整大小
    ui->pushButton_contact_person->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);


}

// 初始化聊天按钮
void Widget::init_pushButton_chat()
{
    // 设置按钮无边框和悬浮效果
    ui->pushButton_chat->setStyleSheet(
        "QPushButton {"
        "    border: none;"
        "    background-color: transparent;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(224, 224, 224, 100);  /* 浅灰色，alpha=100（0-255） */"
        "    border-radius: 4px;"            // 悬浮时圆角效果
        "}"
        );

    // 设置图标并自适应按钮大小
    QIcon chatIcon(":/images/chat.png");  // 替换为实际图标路径
    ui->pushButton_chat->setIcon(chatIcon);
    ui->pushButton_chat->setIconSize(QSize(24, 24));  // 初始图标大小

    // 允许按钮自适应大小
    ui->pushButton_chat->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // 可选：设置按钮固定大小
    // ui->pushButton_chat->setFixedSize(40, 40);
}




void Widget::init_background_color()
{
    // 初始颜色设置（加深版本）
    m_color1 = QColor(255, 200, 220);  // 深一点的粉色（降低绿色分量）
    m_color2 = QColor(180, 210, 255);  // 深一点的蓝色（降低绿色和红色分量）

    // 创建颜色动画1：控制渐变起始色
    m_anim1 = new QPropertyAnimation(this, "color1");
    m_anim1->setDuration(3000);        // 动画持续3秒
    m_anim1->setLoopCount(-1);         // 无限循环
    // 颜色变化路径：深粉 -> 深绿 -> 深粉
    m_anim1->setKeyValueAt(0, QColor(255, 200, 220));
    m_anim1->setKeyValueAt(0.5, QColor(200, 255, 220));  // 更深的绿色
    m_anim1->setKeyValueAt(1, QColor(255, 200, 220));

    // 创建颜色动画2：控制渐变结束色
    m_anim2 = new QPropertyAnimation(this, "color2");
    m_anim2->setDuration(5000);        // 动画持续5秒
    m_anim2->setLoopCount(-1);         // 无限循环
    // 颜色变化路径：深蓝 -> 深青 -> 深蓝
    m_anim2->setKeyValueAt(0, QColor(180, 210, 255));
    m_anim2->setKeyValueAt(0.5, QColor(200, 230, 255));  // 更深的青色
    m_anim2->setKeyValueAt(1, QColor(180, 210, 255));

    // 启动动画
    m_anim1->start();
    m_anim2->start();
}

void Widget::init_treeWidget_friend()
{

}

void Widget::init_treeWidget_group()
{

}

// 在初始化聊天记录ListWidget的函数中（initListWidgetRecord()）添加滚动条灵敏度设置
void Widget::initListWidgetRecord() {
    writeLog("开始初始化聊天记录ListWidget", LOG_INFO);

    // 隐藏滚动条
    ui->listWidget_recieve->setStyleSheet(
        "QListWidget {"
        "   background-color: #f5f5f5; "
        "   color: #333333;"
        "   border: 1px solid #ddd; "
        "   padding: 5px; "
        "   spacing: 8px; "
        "}"
        "QListWidget::item {"
        "   border: none; "
        "   selection-background-color: transparent; "
        "}"
        "QListWidget::item:selected {"
        "   background-color: transparent; "
        "   color: #333333; "
        "   border: none; "
        "}"
        "QScrollBar:vertical {"
        "   width: 0px; "  // 隐藏滚动条
        "}"
        );

    // 安装事件过滤器，捕获滚轮事件
    ui->listWidget_recieve->viewport()->installEventFilter(this);

    // 原有布局模式设置
    ui->listWidget_recieve->setLayoutMode(QListWidget::Batched);
    ui->listWidget_recieve->setBatchSize(10);

    writeLog("聊天记录ListWidget初始化完成", LOG_INFO);
}

void Widget::initFriendList()
{
    // 创建好友列表数据模型
    QStandardItemModel *model = new QStandardItemModel(ui->listView_friend);
    ui->listView_friend->setModel(model);

    // 获取当前登录用户信息（注意：UserManager中没有getName()，应使用getUserId()）
    UserManager &userManager = UserManager::getInstance();
    QString userName = userManager.getName();  // 修正：使用UserManager提供的getUserId()
    if (userName.isEmpty()) {
        writeLog("获取用户名失败，使用默认名称", LOG_WARNING);
        userName = "我自己";
    }


    // 设置列表视图样式（关键修改：增加item的左侧内边距）
    ui->listView_friend->setStyleSheet(
        "QlistView_friend {"
        "    background-color: white;"
        "    outline: none;"
        "    border: none;"
        "}"
        "QlistView_friend::item {"
        "    height: 50px;"
        "    border-bottom: 1px solid #f0f0f0;"
        "    background-color: white;"
        "    color: #333333;"
        "    padding-left: 10px; /* 左侧内边距，使头像和文字右移（可调整数值） */"
        "}"
        "QlistView_friend::item:selected {"
        "    background-color: #ccebff;"
        "    color: #000000;"
        "    border: none;"
        "}"
        "QlistView_friend::item:hover:!selected {"
        "    background-color: rgba(200, 200, 200, 80);"
        "}"
        );

    // 设置默认选中第一行
    if (model->rowCount() > 0) {
        QModelIndex firstIndex = model->index(0, 0);
        ui->listView_friend->setCurrentIndex(firstIndex);
    }

    writeLog("好友列表初始化完成，已添加好友项", LOG_INFO);
}





void Widget::on_pushButton_chat_clicked()
{
    // 切换到tab1（假设tab1是第一个标签页，索引为0）
    ui->tabWidget->setCurrentIndex(0);
}


void Widget::on_pushButton_contact_person_clicked()
{
    // 切换到tab2（假设tab2是第二个标签页，索引为1）
    ui->tabWidget->setCurrentIndex(1);
}

void Widget::mousePressEvent(QMouseEvent *event)
{
    writeLog("鼠标点击事件触发，检查是否点击头像区域", LOG_INFO);  // 新增日志
    if (ui->label_avatar->geometry().contains(event->pos())) {
        writeLog("检测到头像标签点击，打开图片选择对话框", LOG_INFO);  // 保留原有日志并优化
        QString filePath = QFileDialog::getOpenFileName(
            this,
            tr("选择图片"),
            QDir::homePath(),
            tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)")
            );

        if (!filePath.isEmpty()) {
            writeLog(QString("用户选择了图片文件: %1").arg(filePath), LOG_INFO);  // 新增日志
            QPixmap pixmap(filePath);
            if (!pixmap.isNull()) {
                QPixmap roundPixmap(ui->label_avatar->size());
                roundPixmap.fill(Qt::transparent);
                QPainter painter(&roundPixmap);
                painter.setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addEllipse(roundPixmap.rect());
                painter.setClipPath(path);
                painter.drawPixmap(0, 0, pixmap.scaled(
                                             roundPixmap.size(),
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation
                                             ));
                ui->label_avatar->setPixmap(roundPixmap);
                writeLog("头像图片处理为圆形并显示成功", LOG_INFO);  // 新增日志

                saveAvatarPath(filePath);
                writeLog("头像路径已保存到本地文件", LOG_INFO);  // 新增日志
                uploadAvatarToServer(filePath); // 上传到服务器
                writeLog("已触发头像上传到服务器的请求", LOG_INFO);  // 新增日志
            } else {
                writeLog(QString("头像文件无效，无法加载: %1").arg(filePath), LOG_ERROR);  // 优化日志
            }
        } else {
            writeLog("用户取消了头像选择对话框", LOG_INFO);  // 优化日志
        }
    } else {
        writeLog("鼠标点击非头像区域，不处理头像相关操作", LOG_INFO);  // 新增日志
    }

    QWidget::mousePressEvent(event);
}

void Widget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);  // 忽略事件参数
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);  // 启用抗锯齿

    // 绘制区域：稍微缩小避免边框超出窗口
    QRectF windowRect = rect().adjusted(1, 1, -1, -1);
    int radius = 10;  // 圆角半径

    // 绘制渐变背景：使用两个动画颜色
    QLinearGradient gradient(0, 0, width(), height());  // 从左上角到右下角的渐变
    gradient.setColorAt(0, m_color1);  // 起始色
    gradient.setColorAt(1, m_color2);  // 结束色
    painter.setBrush(gradient);        // 设置画刷为渐变

    // 绘制边框：浅灰色，1px宽
    painter.setPen(QPen(QColor(220, 220, 220), 1));

    // 绘制带圆角的矩形作为窗口背景
    painter.drawRoundedRect(windowRect, radius, radius);
}


void Widget::closeEvent(QCloseEvent *event) {
    writeLog("检测到窗口关闭事件，准备执行登出操作", LOG_INFO);
    // 获取当前登录用户ID
    QString userId = UserManager::getInstance().getUserId();
    QString userPhone = UserManager::getInstance().getPhoneNumber();
    writeLog(QString("当前用户ID: %1，执行登出操作").arg(userId), LOG_INFO);

    if (!userId.isEmpty()) {
        // 阻止事件默认处理（先不关闭窗口）
        event->ignore();

        // 执行登出操作
        QThread* logoutThread = new QThread;
        LogoutWorker* logoutWorker = new LogoutWorker(userPhone);

        logoutWorker->moveToThread(logoutThread);
        connect(logoutThread, &QThread::started, logoutWorker, &LogoutWorker::doLogout);
        connect(logoutWorker, &LogoutWorker::finished, logoutThread, &QThread::quit);
        connect(logoutWorker, &LogoutWorker::finished, logoutWorker, &LogoutWorker::deleteLater);
        connect(logoutThread, &QThread::finished, logoutThread, &QThread::deleteLater);

        // 登出结果处理
        connect(logoutWorker, &LogoutWorker::logoutResult, this, [=](bool success){
            if (success) {
                UserManager::getInstance().clearUserInfo();
                writeLog("登出成功，清除用户ID", LOG_INFO);
            } else {
                writeLog("登出失败，仍清除用户ID", LOG_WARNING);
                UserManager::getInstance().clearUserInfo(); // 即使失败也清除，避免状态不一致
            }
        });

        // 线程完成后主动关闭窗口
        connect(logoutThread, &QThread::finished, this, [=](){
            writeLog("登出线程执行完毕，关闭窗口", LOG_INFO);
            // 调用close()触发新的关闭事件，此时userId已清空，会直接关闭
            this->close();
        });

        logoutThread->start();
    } else {
        // 用户未登录，直接关闭
        event->accept();
    }
}



// 读取avatar.txt中的头像路径
QString Widget::readAvatarPath()
{
    QFile file("avatar.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString path = in.readLine().trimmed();  // 读取第一行并去除空格
        file.close();
        // 检查路径是否有效（文件存在）
        if (!path.isEmpty() && QFile::exists(path)) {
            return path;
        }
    }
    // 读取失败或路径无效，返回空
    return "";
}

// 保存头像路径到avatar.txt
void Widget::saveAvatarPath(const QString &path)
{
    QFile file("avatar.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << path;  // 写入路径
        file.close();
    }
}

bool Widget::eventFilter(QObject *watched, QEvent *event)
{
    // 判断事件是否来自头像标签
    if (watched == ui->label_avatar) {
        // 鼠标进入标签时，显示灰色边框
        if (event->type() == QEvent::Enter) {
            ui->label_avatar->setStyleSheet(
                "QLabel {"
                "   border-radius: 30px;"
                "   background-color: white;"
                "   border: 2px solid #ffffff; /* 灰色边框 */"
                "}"
                );
            return true; // 已处理事件
        }
        // 鼠标离开标签时，恢复透明边框
        else if (event->type() == QEvent::Leave) {
            ui->label_avatar->setStyleSheet(
                "QLabel {"
                "   border-radius: 30px;"
                "   background-color: white;"
                "}"
                );
            return true; // 已处理事件
        }
    }
    // 新增：处理textEdit_send的回车键事件
    if (watched == ui->textEdit_send && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        // 捕获回车键（不含Shift+Enter，允许换行）
        if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
            && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            // 触发发送消息
            if (ui->pushButton_send->isEnabled()) {
                on_pushButton_send_clicked();
                writeLog("textEdit_send中按下回车键触发发送消息");
                return true; // 阻止事件继续传递（避免输入换行）
            }
        }
    }
    // 判断是否是listWidget_recieve的视口产生的滚轮事件
    if (watched == ui->listWidget_recieve->viewport() &&
        event->type() == QEvent::Wheel) {

        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

        // 获取滚动条
        QScrollBar *scrollBar = ui->listWidget_recieve->verticalScrollBar();
        if (!scrollBar) return QWidget::eventFilter(watched, event);

        // 计算滚动距离（自定义灵敏度）
        int baseScroll = 1; // 基础滚动量
        int scrollAmount = (wheelEvent->angleDelta().y() / 120) * baseScroll;
        // 向下滚动时angleDelta为负，向上为正，直接累加即可
        scrollBar->setValue(scrollBar->value() - scrollAmount);

        // 阻止事件继续传播（避免默认滚动行为）
        return true;
    }
    // 其他事件交给父类处理
    return QWidget::eventFilter(watched, event);
}

// 核心修改：统一消息时间渲染，确保新消息时间居中
void Widget::displayMessages(const QJsonArray& messages) {
    QString myId = UserManager::getInstance().getUserId();
    bool isGroupChat = (m_currentFriendId == "0000"); // 判断是否为群聊

    for (const auto& msgVal : messages) {
        QJsonObject msg = msgVal.toObject();
        QString fromUserId = msg["fromUserId"].toString();
        bool isMyMessage = (fromUserId == myId);
        QString userName = getUserNameById(fromUserId); // 获取发送者用户名

        // 创建消息项Widget，传递群聊标识和用户名
        MessageItemWidget* item = new MessageItemWidget(
            isMyMessage,
            msg["time"].toString(),
            msg["content"].toString(),
            fromUserId,
            isGroupChat,         // 新增：群聊标识
            userName,            // 新增：发送者用户名
            ui->listWidget_recieve
            );

        QListWidgetItem* listItem = new QListWidgetItem(ui->listWidget_recieve);
        listItem->setSizeHint(item->sizeHint());
        ui->listWidget_recieve->setItemWidget(listItem, item);
    }

    // 强制滚动到底部，确保新消息可见
    // 先禁用批处理模式，确保所有项都已布局
    ui->listWidget_recieve->setLayoutMode(QListWidget::SinglePass);
    // 立即更新界面布局
    ui->listWidget_recieve->updateGeometry();
    ui->listWidget_recieve->repaint();
    // 滚动到底部
    ui->listWidget_recieve->scrollToBottom();
    // 恢复批处理模式
    ui->listWidget_recieve->setLayoutMode(QListWidget::Batched);
}


// 新增：时间显示格式化函数
QString Widget::getDisplayTime(const QDateTime &time) {
    QDate currentDate = QDate::currentDate();
    QDate msgDate = time.date();

    // 使用24小时制(HH)格式化时间
    if (msgDate == currentDate) {
        return time.toString("HH:mm"); // 24小时制显示
    } else if (msgDate.daysTo(currentDate) == 1) {
        return QString("昨天 %1").arg(time.toString("HH:mm"));
    } else if (msgDate.year() == currentDate.year()) {
        return time.toString("MM-dd HH:mm");
    } else {
        return time.toString("yyyy-MM-dd HH:mm");
    }
}

void Widget::uploadAvatarToServer(const QString &filePath)
{
    writeLog(QString("开始上传头像文件: %1 到服务器").arg(filePath), LOG_INFO);

    // 检查文件是否存在
    if (!QFile::exists(filePath)) {
        writeLog(QString("头像文件不存在: %1").arg(filePath), LOG_ERROR);
        return;
    }

    QFile file(filePath);
    // 尝试打开文件
    if (!file.open(QIODevice::ReadOnly)) {
        writeLog(QString("无法打开头像文件（错误: %1）: %2")
                     .arg(file.errorString())
                     .arg(filePath), LOG_ERROR);
        return;
    }

    // 读取文件数据并转换为Base64
    QByteArray imageData = file.readAll();
    file.close(); // 及时关闭文件

    writeLog(QString("成功读取头像文件，大小: %1 字节").arg(imageData.size()), LOG_INFO);

    // 检查文件大小（可选：添加大小限制）
    if (imageData.size() > 5 * 1024 * 1024) { // 5MB限制
        writeLog("头像文件超过5MB限制，取消上传", LOG_ERROR);
        return;
    }

    QString base64Data = imageData.toBase64();
    writeLog(QString("头像文件转换为Base64后长度: %1 字符").arg(base64Data.length()), LOG_INFO);

    // 构建请求数据
    QString userId = UserManager::getInstance().getUserId();
    if (userId.isEmpty()) {
        writeLog("当前用户未登录，无法上传头像", LOG_ERROR);
        return;
    }

    QJsonObject request;
    request["user_id"] = userId; // 当前登录用户ID
    request["avatar_data"] = base64Data;

    // 发送POST请求
    QUrl url("http://101.37.68.162:8080/api/upload_avatar");
    QNetworkRequest networkRequest(url);
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    writeLog(QString("发送头像上传请求到: %1").arg(url.toString()), LOG_INFO);

    QNetworkReply* reply = m_networkManager->post(
        networkRequest,
        QJsonDocument(request).toJson()
        );

    // 设置超时处理（5秒）
    QTimer::singleShot(5000, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
            qWarning() << "头像上传请求超时";
        }
    });

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            writeLog(QString("收到头像上传响应，数据大小: %1 字节").arg(responseData.size()), LOG_INFO);

            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
            if (doc.isNull()) {
                writeLog(QString("解析头像上传响应失败: %1（偏移量: %2）")
                             .arg(parseError.errorString())
                             .arg(parseError.offset), LOG_ERROR);
            } else {
                QJsonObject response = doc.object();
                if (response["success"].toBool()) {
                    writeLog("头像上传到服务器成功", LOG_INFO);
                } else {
                    writeLog(QString("头像上传失败（服务器返回错误）: %1")
                                 .arg(response["error"].toString()), LOG_ERROR);
                }
            }
        } else {
            writeLog(QString("头像上传网络错误（错误码: %1）: %2")
                         .arg(reply->error())
                         .arg(reply->errorString()), LOG_ERROR);
        }
        reply->deleteLater();
        writeLog("头像上传请求处理完成", LOG_INFO);
    });
}

void Widget::onLoginSuccess()
{
    writeLog("登录成功回调触发，开始处理用户信息和头像", LOG_INFO);  // 新增日志
    // 保存用户信息
    QString userId = UserManager::getInstance().getUserId();
    writeLog(QString("当前登录用户ID: %1，准备获取头像").arg(userId), LOG_INFO);  // 新增日志

    // 获取并显示头像
    fetchAvatarFromServer(userId);
    writeLog("已触发从服务器获取头像的请求", LOG_INFO);  // 新增日志
}
// 从服务器获取头像
void Widget::fetchAvatarFromServer(const QString &userId) {
    writeLog(QString("开始从服务器获取用户ID为 %1 的头像").arg(userId), LOG_INFO);  // 新增日志

    QUrl url("http://101.37.68.162:8080/api/get_avatar");
    QUrlQuery query;
    query.addQueryItem("user_id", userId);
    url.setQuery(query);
    writeLog(QString("构建头像请求URL: %1").arg(url.toString()), LOG_INFO);  // 新增日志

    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager->get(request);
    writeLog("已发送获取头像的GET请求", LOG_INFO);  // 新增日志

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray imageData = reply->readAll();
            writeLog(QString("成功接收头像数据，大小: %1 字节").arg(imageData.size()), LOG_INFO);  // 新增日志

            QPixmap pixmap;
            if (pixmap.loadFromData(imageData)) {
                // 处理为圆形头像
                QPixmap roundPixmap(ui->label_avatar->size());
                roundPixmap.fill(Qt::transparent);
                QPainter painter(&roundPixmap);
                painter.setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addEllipse(roundPixmap.rect());
                painter.setClipPath(path);
                painter.drawPixmap(0, 0, pixmap.scaled(
                                             roundPixmap.size(),
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation
                                             ));
                ui->label_avatar->setPixmap(roundPixmap);
                writeLog("从服务器获取头像成功，并已处理为圆形显示", LOG_INFO);  // 新增日志
            } else {
                writeLog("服务器返回的头像数据无法解析为图片", LOG_ERROR);  // 新增日志
            }
        } else {
            writeLog(QString("获取头像失败: %1（错误码：%2）")
                         .arg(reply->errorString())
                         .arg(reply->error()), LOG_ERROR);  // 新增日志
            // 使用默认头像
            writeLog("获取头像失败，将使用默认头像", LOG_WARNING);  // 新增日志
        }
        reply->deleteLater();
    });
}


void Widget::onSendMessageFinished()
{

    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            QJsonObject response = doc.object();
            if (response["success"].toBool()) {

                writeLog("消息发送成功", LOG_INFO);
                return;
            } else {
                writeLog(QString("消息发送失败：%1").arg(response["error"].toString()), LOG_ERROR);
                return;
            }
        }
    }

    writeLog(QString("消息发送失败：%1").arg(reply->errorString()), LOG_ERROR);
    writeLog("消息发送完成回调触发", LOG_INFO);

    // 新增：发送成功后立即拉取最新消息
    if (!m_currentFriendId.isEmpty()) {
        writeLog("消息发送成功，立即拉取最新消息", LOG_INFO);
        fetchNewMessages(); // 强制获取最新消息
    }
}



