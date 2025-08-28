#include "loginstatewidget.h"
#include "ui_loginstatewidget.h"
#include <QLabel>         // 标签控件，用于显示文本
#include <QPushButton>    // 按钮控件
#include <QHBoxLayout>    // 水平布局管理器
#include <QVBoxLayout>    // 垂直布局管理器
#include <QMouseEvent>    // 鼠标事件类
#include <QCursor>        // 光标类
#include <QPainter>       // 绘图类
#include <QDateTime>      // 用于日志时间戳
#include <QFile>
#include <QDir>
// 新增：日志工具函数
static void writeLoginStateLog(const QString& message, const QString& type = "INFO") {
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
    QString fileName = logDirPath + QString("/登录状态窗口日志_%1.log").arg(dateStr);

    QString logMsg = QString("[%1] %2: LoginStateWidget - %3")
                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                         .arg(type)
                         .arg(message);
    qDebug() << logMsg;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << logMsg << "\n";
        file.close();
    } else {
        qWarning() << "无法打开日志文件：" << fileName << "，错误：" << file.errorString();
    }
}

// 构造函数：初始化登录状态提示窗口
LoginStateWidget::LoginStateWidget(QWidget *parent)
    : QWidget(parent)  // 允许传入父窗口，但不强制关联
    , ui(new Ui::LoginStateWidget)
{
    ui->setupUi(this);
    writeLoginStateLog("初始化登录状态窗口");
    // 设置窗口为无边框对话框样式
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    // 设置窗口背景透明（用于实现圆角效果）
    setAttribute(Qt::WA_TranslucentBackground);
    // 固定窗口大小为登录窗口的90%宽度，高度170px
    setFixedSize(325 * 0.9, 170);
    // 设置为应用级模态窗口，阻塞其他窗口交互
    setWindowModality(Qt::ApplicationModal);
    writeLoginStateLog("已设置窗口属性：无边框、透明背景、固定大小、应用级模态");

    // 手动关联父窗口（仅用于定位，不影响显示）
    if (parent) {
        setParent(parent, Qt::Dialog);  // 第二个参数确保窗口特性正确
    }

    // 初始化界面控件
    initWidgets();
    writeLoginStateLog("登录状态窗口初始化完成");
}

// 析构函数：释放UI资源
LoginStateWidget::~LoginStateWidget()
{
    writeLoginStateLog("登录状态窗口开始销毁");
    delete ui;
    writeLoginStateLog("登录状态窗口销毁完成");
}

// 初始化界面控件和布局
void LoginStateWidget::initWidgets()
{
    writeLoginStateLog("开始初始化界面控件");
    // 1. 标题标签（显示"登录失败"）
    QLabel* titleLabel = new QLabel("登录失败", this);
    titleLabel->setStyleSheet(
        "QLabel {"
        "   font-family: 'Microsoft YaHei';"  // 字体：微软雅黑
        "   font-size: 15px;"                 // 字号：15px
        "   color: #333333;"                  // 颜色：深灰色
        "   font-weight: bold;"               // 加粗显示
        "}"
        );

    // 2. 提示信息标签（显示错误原因）
    QLabel* msgLabel = new QLabel("账号或密码错误，请重新输入。", this);
    msgLabel->setStyleSheet(
        "QLabel {"
        "   font-family: 'Microsoft YaHei';"  // 字体：微软雅黑
        "   font-size: 14px;"                 // 字号：14px
        "   color: #666666;"                  // 颜色：中灰色
        "}"
        );
    msgLabel->setObjectName("msgLabel");

    // 3. 忘记密码按钮
    QPushButton* forgetBtn = new QPushButton("忘记密码", this);
    forgetBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #0099ff;"       // 背景色：蓝色
        "   color: white;"                    // 文字色：白色
        "   border: none;"                    // 无边框
        "   border-radius: 5px;"              // 圆角：5px
        "   padding: 2px 15px;"               // 内边距
        "   font-family: 'Microsoft YaHei';"  // 字体：微软雅黑
        "   font-size: 14px;"                 // 字号：14px
        "}"
        "QPushButton:hover {"                 // 鼠标悬停状态
        "   background-color: #0088ee;"       // 背景色：深蓝色
        "}"
        "QPushButton:pressed {"               // 鼠标按下状态
        "   background-color: #0077dd;"       // 背景色：更深蓝色
        "}"
        );
    forgetBtn->setFixedSize(90, 30);  // 固定按钮大小：90x30px

    // 4. 重新登录按钮
    QPushButton* reloginBtn = new QPushButton("重新登录", this);
    reloginBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: white;"         // 背景色：白色
        "   color: #333333;"                  // 文字色：深灰色
        "   border: 1px solid #CCCCCC;"       // 边框：1px浅灰色
        "   border-radius: 5px;"              // 圆角：5px
        "   padding: 2px 15px;"               // 内边距
        "   font-family: 'Microsoft YaHei';"  // 字体：微软雅黑
        "   font-size: 14px;"                 // 字号：14px
        "}"
        "QPushButton:hover {"                 // 鼠标悬停状态
        "   border-color: #999999;"           // 边框色：深灰色
        "}"
        );
    reloginBtn->setFixedSize(90, 30);  // 固定按钮大小：90x30px

    // 5. 关闭按钮（右上角×）
    QPushButton* closeBtn = new QPushButton("×", this);
    closeBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: transparent;"   // 背景透明
        "   color: #666666;"                  // 文字色：中灰色
        "   border: none;"                    // 无边框
        "   font-size: 18px;"                 // 字号：18px
        "   min-width: 30px;"                 // 最小宽度：30px（改为min-width）
        "   min-height: 30px;"                // 最小高度：30px（改为min-height）
        "   padding: 0px;"                    // 移除内边距，确保点击区域完整
        "}"

        );
    // 确保光标设置在样式表之后，避免被覆盖
    closeBtn->setCursor(Qt::PointingHandCursor);
    // 连接关闭按钮点击信号到关闭槽函数
    connect(closeBtn, &QPushButton::clicked, this, &LoginStateWidget::close);
    writeLoginStateLog("已创建所有界面控件并设置样式");

    // 按钮水平布局 - 调整按钮位置
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);  // 左侧添加空白拉伸，使按钮整体右移
    btnLayout->addWidget(forgetBtn);
    btnLayout->addSpacing(10);  // 按钮间距：10px
    btnLayout->addWidget(reloginBtn);
    btnLayout->addStretch(0);  // 右侧保留少量空白，避免贴边

    // 主布局（垂直布局）
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 创建关闭按钮的水平布局
    QHBoxLayout* closeLayout = new QHBoxLayout();
    closeLayout->addStretch();  // 左侧拉伸，挤压按钮右移
    closeLayout->addWidget(closeBtn);
    closeLayout->addSpacing(-13);  // 右侧减少间距，使按钮右移

    // 将关闭按钮布局添加到主布局
    mainLayout->addLayout(closeLayout, 0);
    mainLayout->addSpacing(1);

    // 添加文本区域控件
    mainLayout->addWidget(titleLabel, 0, Qt::AlignLeft | Qt::AlignTop);
    mainLayout->addWidget(msgLabel, 0, Qt::AlignLeft);
    mainLayout->addSpacing(5);

    // 添加拉伸空间，使按钮区域靠下
    mainLayout->addStretch(1);

    // 添加按钮区域
    mainLayout->addLayout(btnLayout);
    mainLayout->addSpacing(15);

    // 设置布局边距（上、左、下、右）
    mainLayout->setContentsMargins(20, 5, 15, 15);
    writeLoginStateLog("界面布局设置完成");

    // 连接忘记密码按钮信号
    connect(closeBtn, &QPushButton::clicked, this, [=](){
        writeLoginStateLog("用户点击忘记密码按钮");
        emit reloginRequested();  // 发送重新登录信号
        close();  // 点击后关闭窗口
    });

    // 连接重新登录按钮信号
    connect(reloginBtn, &QPushButton::clicked, this, [=](){
        writeLoginStateLog("用户点击重新登录按钮");
        emit reloginRequested();  // 发送重新登录信号
        close();  // 关闭错误窗口
    });
}

// 重绘事件：绘制窗口背景和边框
void LoginStateWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);  // 忽略事件参数

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);  // 启用抗锯齿

    // 绘制区域（向内缩进1px，避免边框超出窗口）
    QRectF windowRect = rect().adjusted(1, 1, -1, -1);
    int radius = 10;  // 圆角半径：10px

    // 设置画刷为白色，画笔为浅灰色（边框）
    painter.setBrush(Qt::white);
    painter.setPen(QPen(QColor(220, 220, 220), 1));

    // 绘制带边框的圆角矩形
    painter.drawRoundedRect(windowRect, radius, radius);
}

// 移动事件：确保窗口始终居中于父窗口
void LoginStateWidget::moveEvent(QMoveEvent *event)
{
    if (parentWidget()) {  // 如果存在父窗口
        // 计算父窗口中心点
        QPoint parentCenter = parentWidget()->geometry().center();
        // 移动窗口到父窗口中心
        move(parentCenter.x() - width()/2, parentCenter.y() - height()/2);
        writeLoginStateLog("窗口已移动到父窗口中心位置");
    }
    QWidget::moveEvent(event);  // 调用父类的移动事件处理
}
