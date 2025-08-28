#include "widget.h"
#include "ui_widget.h"
#include "networkmanager.h"
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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QBuffer>
#include <QDataStream>
#include <QTcpSocket>
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


    init_label_name();

    // 初始化高效的 QListView 聊天区域
    ui->listWidget_recieve->hide(); // 隐藏旧的列表控件
    
    m_chatListView = new QListView(ui->tab);
    m_chatListView->setObjectName("chatListView");
    m_chatListView->setFrameShape(QFrame::NoFrame);
    m_chatListView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_chatListView->setSelectionMode(QAbstractItemView::SingleSelection); // Changed from NoSelection to facilitate clicks
    m_chatListView->setUniformItemSizes(false); // 允许不同高度
    m_chatListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_chatListView->setFocusPolicy(Qt::StrongFocus);
    m_chatListView->setSpacing(0);
    
    m_chatModel = new QStandardItemModel(this);
    m_chatListView->setModel(m_chatModel);
    
    m_chatDelegate = new ChatItemDelegate(this);
    m_chatListView->setItemDelegate(m_chatDelegate);
    
    // 布局设置（尝试在不破坏原有布局的情况下插入）
    if (ui->tab->layout()) {
        ui->tab->layout()->addWidget(m_chatListView);
    } else {
        m_chatListView->setGeometry(ui->listWidget_recieve->geometry());
    }
    
    initListWidgetRecord(); 


    writeLog("Widget主窗口初始化完成", LOG_INFO);

    // 获取当前登录用户信息
    m_myUserId = UserManager::getInstance().getUserId();
    m_myUserName = UserManager::getInstance().getName();
    m_myPhone = UserManager::getInstance().getPhoneNumber();

    // 使用 NetworkManager 进行通信
    connect(&NetworkManager::instance(), &NetworkManager::newMessage, this, &Widget::onNewMessageReceived);
    connect(&NetworkManager::instance(), &NetworkManager::historyMessagesResponse, this, &Widget::displayMessages);
    connect(&NetworkManager::instance(), &NetworkManager::allUsersResponse, this, [this](const QJsonArray& users) {
        QTreeWidgetItem *rootItem = ui->treeWidget_friend->topLevelItem(0);
        if (rootItem) rootItem->takeChildren();
        m_chatListModel->clear();

        for (const QJsonValue& userVal : users) {
            QJsonObject user = userVal.toObject();
            QString userId = user["user_id"].toString();
            QString userName = user["name"].toString();
            QString userPhone = user["phone"].toString();
            m_userIdToName[userId] = userName;

            if (rootItem) {
                QTreeWidgetItem *userItem = new QTreeWidgetItem(rootItem);
                userItem->setText(0, userName);
                userItem->setData(0, Qt::UserRole, userId);
                userItem->setData(0, Qt::UserRole + 1, userPhone);
                userItem->setIcon(0, QIcon(":/images/QQ.png"));
            }

            QStandardItem *listItem = new QStandardItem();
            listItem->setText(userName);
            listItem->setData(userId, Qt::UserRole);
            listItem->setData(userPhone, Qt::UserRole + 1);
            listItem->setIcon(QIcon(":/images/QQ.png"));
            m_chatListModel->appendRow(listItem);
        }
        QStandardItem *groupItem = new QStandardItem("所有人聊天群");
        groupItem->setData("0000", Qt::UserRole);
        groupItem->setData("group", Qt::UserRole + 1);
        m_chatListModel->appendRow(groupItem);
        writeLog(QString("NetworkManager 成功获取 %1 个用户资料").arg(users.size()), LOG_INFO);
    });
    connect(&NetworkManager::instance(), &NetworkManager::allAvatarsResponse, this, [this](const QJsonArray& avatars) {
        for (const QJsonValue& val : avatars) {
            QString userId = val.toObject()["user_id"].toString();
            if (!userId.isEmpty()) {
                fetchAvatarFromServer(userId);
            }
        }
    });
    connect(&NetworkManager::instance(), &NetworkManager::avatarResponse, this, [this](const QString& userId, bool success, const QByteArray& imgData) {
        if (!success) return;
        QPixmap pixmap;
        if (pixmap.loadFromData(imgData)) {
            // Check if it's my own avatar
            if (userId == m_myUserId) {
                QPixmap roundPixmap(ui->label_avatar->size());
                roundPixmap.fill(Qt::transparent);
                QPainter painter(&roundPixmap);
                painter.setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addEllipse(roundPixmap.rect());
                painter.setClipPath(path);
                painter.drawPixmap(0, 0, pixmap.scaled(roundPixmap.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                ui->label_avatar->setPixmap(roundPixmap);
                writeLog("NetworkManager 成功获取个人头像", LOG_INFO);
            }
            // Update friend list
            m_userIdToAvatar[userId] = pixmap;
            for (int i = 0; i < m_chatListModel->rowCount(); ++i) {
                if (m_chatListModel->item(i)->data(Qt::UserRole).toString() == userId) {
                    m_chatListModel->item(i)->setIcon(QIcon(pixmap));
                }
            }
            QTreeWidgetItem *rootItem = ui->treeWidget_friend->topLevelItem(0);
            if (rootItem) {
                for (int i = 0; i < rootItem->childCount(); ++i) {
                    if (rootItem->child(i)->data(0, Qt::UserRole).toString() == userId) {
                        rootItem->child(i)->setIcon(0, QIcon(pixmap));
                    }
                }
            }
        }
    });
    connect(&NetworkManager::instance(), &NetworkManager::disconnected, this, &Widget::onMainSocketDisconnected);
    connect(&NetworkManager::instance(), &NetworkManager::errorOccurred, this, [this](const QString& err){
        writeLog("网络错误: " + err, LOG_ERROR);
    });
    connect(&NetworkManager::instance(), &NetworkManager::connected, this, &Widget::onMainSocketConnected);
    connect(&NetworkManager::instance(), &NetworkManager::newMessage, this, &Widget::onNewMessageReceived);
    connect(&NetworkManager::instance(), &NetworkManager::friendRequest, this, &Widget::onNewMessageReceived);

    // 立即执行身份识别（因为此时已经登录，连接已由 load 建立）
    if (NetworkManager::instance().isConnected()) {
        onMainSocketConnected();
    }

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
    connect(ui->pushButton_image, &QPushButton::clicked,
            this, &Widget::on_pushButton_image_clicked);
    writeLog("发送按钮和图片按钮点击信号连接完成", LOG_INFO);


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
    writeLog("开始获取所有用户头像信息 (NetworkManager)", LOG_INFO);
    QJsonObject json;
    json["type"] = "get_allAvatar";
    NetworkManager::instance().sendPacket(json);
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
    writeLog("开始获取所有用户信息 (NetworkManager)", LOG_INFO);
    QJsonObject json;
    json["type"] = "get_all_users";
    NetworkManager::instance().sendPacket(json);
}
// 处理ListView好友项点击
void Widget::onFriendListViewItemClicked(const QModelIndex &index) {
    qDebug() << "onFriendListViewItemClicked triggered for index:" << index.row();
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
    m_currentFriendId = item->data(Qt::UserRole).toString().trimmed();
    m_currentFriendName = item->text();
    QString friendPhone = item->data(Qt::UserRole + 1).toString();

    writeLog(QString("开始与 %1 (ID: %2) 通信").arg(m_currentFriendName).arg(m_currentFriendId), LOG_INFO);

    // 清空聊天框
    writeLog("清空聊天记录模型", LOG_INFO);
    m_chatModel->clear();
    m_lastTimeStr = ""; // 重置时间记录
    m_localMessageIds.clear(); // 切换好友时清空本地已读ID，防止无法拉取历史记录
    
    ui->textEdit_send->clear();

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
    QString message = ui->textEdit_send->toPlainText().trimmed();
    if (message.isEmpty() || m_currentFriendId.isEmpty()) return;

    if (!NetworkManager::instance().isConnected()) { // 使用 NetworkManager 的连接状态
        writeLog("发送失败：TCP 未连接", LOG_ERROR);
        return;
    }

    QJsonObject msgJson;
    msgJson["type"] = "send_message";
    msgJson["fromUserId"] = m_myUserId;
    msgJson["toUserId"] = m_currentFriendId;
    msgJson["content"] = message; // Changed 'body' to 'message'
    
    NetworkManager::instance().sendPacket(msgJson); // 使用 NetworkManager 发送
    writeLog(QString("TCP发送消息到 %1: %2").arg(m_currentFriendId).arg(message), LOG_INFO);

    // 本地立即显示
    QJsonArray arr;
    QJsonObject localMsg;
    localMsg["fromUserId"] = m_myUserId;
    localMsg["content"] = message;
    localMsg["time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    arr.append(localMsg);
    displayMessages(arr);

    ui->textEdit_send->clear();
}

void Widget::on_pushButton_image_clicked() {
    if (m_currentFriendId.isEmpty()) return;

    QString filePath = QFileDialog::getOpenFileName(
        this, tr("选择图片"), QDir::homePath(),
        tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)")
    );

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;

    QByteArray imageData = file.readAll();
    file.close();

    // 压缩大图
    if (imageData.size() > 1024 * 1024) {
        QPixmap pix;
        pix.loadFromData(imageData);
        pix = pix.scaled(800, 800, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QBuffer buffer(&imageData);
        buffer.open(QIODevice::WriteOnly);
        pix.save(&buffer, "JPG", 80);
    }

    sendImageMessage(imageData.toBase64());
}

void Widget::sendImageMessage(const QString& base64Data) {
    if (!NetworkManager::instance().isConnected()) return;

    QJsonObject msgJson;
    msgJson["type"] = "send_message";
    msgJson["fromUserId"] = m_myUserId;
    msgJson["toUserId"] = m_currentFriendId;
    msgJson["content"] = "base64:" + base64Data;
    
    NetworkManager::instance().sendPacket(msgJson);

    QJsonArray arr;
    QJsonObject localMsg;
    localMsg["fromUserId"] = m_myUserId;
    localMsg["content"] = "base64:" + base64Data;
    localMsg["time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    arr.append(localMsg);
    displayMessages(arr);
}

void Widget::onMainSocketConnected() {
    writeLog("正在同步登录身份...", LOG_INFO);
    QJsonObject json;
    json["type"] = "identify";
    json["userId"] = m_myUserId;
    
    NetworkManager::instance().sendPacket(json);
}

void Widget::onMainSocketDisconnected() {
    writeLog("TCP 主连接已断开", LOG_WARNING);
}



// 我们需要一个新的槽函数来适配 NetworkManager 的信号
void Widget::onNewMessageReceived(const QJsonObject& json) {
    QString type = json["type"].toString();

    if (type == "new_message") {
        QString fromId = json["fromUserId"].toString().trimmed();
        QString toId = json["toUserId"].toString().trimmed();
        bool isGroup = json["isGroup"].toBool() || (toId == "0000");

        bool shouldDisplay = false;
        if (isGroup) {
            if (m_currentFriendId == "0000") shouldDisplay = true;
        } else {
            if (fromId == m_currentFriendId) shouldDisplay = true;
        }

        if (shouldDisplay) {
            QJsonArray arr;
            arr.append(json);
            displayMessages(arr);
        }
    } else if (type == "new_friend_request_push") {
         writeLog("收到新的好友申请推送", LOG_INFO);
    }
}




// fetchNewMessages 和 onMessagesFetched 已被 WebSocket 替换内容删除

// onGetAllUsersFinished 已由 getAllUsers 内部实现替代



// 处理TreeWidget双击事件
void Widget::onTreeWidgetFriendItemDoubleClicked(QTreeWidgetItem *item, int column) {
    qInfo() << "Tree widget item double clicked!" << item->text(0);
    Q_UNUSED(column);
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
    m_chatListView->viewport()->installEventFilter(this);

    // 原有布局模式设置
    ui->listWidget_recieve->setLayoutMode(QListWidget::Batched);
    ui->listWidget_recieve->setBatchSize(10);

    writeLog("聊天记录ListWidget初始化完成", LOG_INFO);
}

void Widget::initFriendList()
{
    // 创建好友列表数据模型
    m_chatListModel = new QStandardItemModel(ui->listView_friend);
    ui->listView_friend->setModel(m_chatListModel);

    // 获取当前登录用户信息（注意：UserManager中没有getName()，应使用getUserId()）
    UserManager &userManager = UserManager::getInstance();
    QString userName = userManager.getName(); 
    if (userName.isEmpty()) {
        writeLog("获取用户名失败，使用默认名称", LOG_WARNING);
        userName = "我自己";
    }

    // 设置列表视图样式
    ui->listView_friend->setStyleSheet(
        "QListView {"
        "    background-color: white;"
        "    outline: none;"
        "    border: none;"
        "}"
        "QListView::item {"
        "    height: 50px;"
        "    border-bottom: 1px solid #f0f0f0;"
        "    background-color: white;"
        "    color: #333333;"
        "    padding-left: 10px;"
        "}"
        "QListView::item:selected {"
        "    background-color: #ccebff;"
        "    color: #000000;"
        "    border: none;"
        "}"
        "QListView::item:hover:!selected {"
        "    background-color: rgba(200, 200, 200, 80);"
        "}"
        );

    // 设置默认选中第一行
    if (m_chatListModel->rowCount() > 0) {
        QModelIndex firstIndex = m_chatListModel->index(0, 0);
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
    if (event->type() == QEvent::MouseButtonPress) {
        qDebug() << "MousePress detected on:" << watched->objectName() << "Type:" << watched->metaObject()->className();
    }
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
                writeLog("textEdit_send中按下回车键触发发送消息", LOG_INFO);
                return true; // 阻止事件继续传递（避免输入换行）
            } else {
                writeLog("textEdit_send中按下回车键，但发送按钮不可用");
            }
        }
    }
    // head label avatar filter remains above...
    // 其他事件交给父类处理
    return QWidget::eventFilter(watched, event);
}

// 核心修改：统一消息时间渲染，确保新消息时间居中
void Widget::displayMessages(const QJsonArray& messages) {
    QString myId = UserManager::getInstance().getUserId();
    bool isGroupChat = (m_currentFriendId == "0000");

    for (const auto& msgVal : messages) {
        QJsonObject msg = msgVal.toObject();
        QString msgId = msg["id"].toString().trimmed();
        
        // 防止重复显示相同 ID 的消息
        if (!msgId.isEmpty()) {
            if (m_localMessageIds.contains(msgId)) continue;
            m_localMessageIds.insert(msgId);
            // 更新最后一条消息 ID
            if (m_lastMessageIds[m_currentFriendId].toLongLong() < msgId.toLongLong()) {
                m_lastMessageIds[m_currentFriendId] = msgId;
            }
        }

        QString fromUserId = msg["fromUserId"].toString().trimmed();
        bool isMyMessage = (fromUserId == myId);
        QString userName = getUserNameById(fromUserId);
        QString timeStr = msg["time"].toString();

        QString content = msg["content"].toString();
        bool isImage = content.startsWith("base64:");
        if (isImage) {
            content = content.mid(7); 
        }

        // 添加时间分隔符 (如果两条消息间隔超过 2 分钟)
        QDateTime currentMsgTime = QDateTime::fromString(timeStr, "yyyy-MM-dd hh:mm:ss");
        if (!currentMsgTime.isValid()) {
            currentMsgTime = QDateTime::fromString(timeStr, "yyyy-MM-dd HH:mm:ss");
        }

        if (m_lastTimeStr.isEmpty() || 
            (currentMsgTime.isValid() && QDateTime::fromString(m_lastTimeStr, "yyyy-MM-dd hh:mm:ss").secsTo(currentMsgTime) > 120)) {
            
            QStandardItem* timeItem = new QStandardItem();
            timeItem->setData(true, ChatItemDelegate::IsHeaderRole);
            timeItem->setData(getDisplayTime(currentMsgTime), ChatItemDelegate::TimeRole);
            m_chatModel->appendRow(timeItem);
            m_lastTimeStr = timeStr;
        }

        // 创建数据项
        QStandardItem* msgItem = new QStandardItem();
        msgItem->setData(fromUserId, ChatItemDelegate::FromUserIdRole);
        msgItem->setData(userName, ChatItemDelegate::UserNameRole);
        msgItem->setData(content, ChatItemDelegate::ContentRole);
        msgItem->setData(timeStr, ChatItemDelegate::TimeRole);
        msgItem->setData(isMyMessage, ChatItemDelegate::IsMyMessageRole);
        msgItem->setData(isGroupChat, ChatItemDelegate::IsGroupChatRole);
        msgItem->setData(isImage, ChatItemDelegate::IsImageRole);
        msgItem->setData(false, ChatItemDelegate::IsHeaderRole);
        
        m_chatModel->appendRow(msgItem);
    }

    // 滚动到底部
    QTimer::singleShot(50, this, [this](){
        m_chatListView->scrollToBottom();
    });
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

void Widget::uploadAvatarToServer(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    QByteArray imageData = file.readAll();
    file.close();

    QJsonObject json;
    json["type"] = "upload_avatar";
    json["user_id"] = m_myUserId;
    json["avatar_data"] = QString(imageData.toBase64());
    
    NetworkManager::instance().sendPacket(json);
    writeLog("已通过 NetworkManager 发送头像上传请求", LOG_INFO);
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
    QJsonObject json;
    json["type"] = "get_avatar";
    json["user_id"] = userId;
    NetworkManager::instance().sendPacket(json);
}


// onSendMessageFinished 已废弃

void Widget::fetchNewMessages() {
    if (m_currentFriendId.isEmpty()) return;

    writeLog(QString("NetworkManager 拉取与 %1 的历史消息").arg(m_currentFriendName), LOG_INFO);

    QJsonObject json;
    json["type"] = "get_messages";
    json["fromUserId"] = m_myUserId;
    json["toUserId"] = m_currentFriendId;
    json["lastMessageId"] = m_lastMessageIds.value(m_currentFriendId, "0");

    NetworkManager::instance().sendPacket(json);
}




