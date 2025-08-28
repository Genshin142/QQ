#include "enrollstatewidget.h"
#include "ui_enrollstatewidget.h"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QApplication>
#include <QDateTime>
#include<QFile>
#include<QDir>
// 日志工具函数
static void writeEnrollStateLog(const QString& message, const QString& type = "INFO") {
    // 1. 构建根目录下的"日志"文件夹路径
    QString logDirPath = QCoreApplication::applicationDirPath() + "/日志";
    // 2. 检查并创建"日志"文件夹（不存在则创建）
    QDir logDir(logDirPath);
    if (!logDir.exists()) {
        if (!logDir.mkpath(".")) { // 创建文件夹（包括必要的父目录）
            qWarning() << "无法创建日志文件夹：" << logDirPath;
            return; // 创建失败则直接返回，避免后续错误
        }
    }

    // 3. 构建完整日志文件路径（日志文件夹 + 固定文件名）
    QString fileName = logDirPath + "/注册状态窗口日志.log";

    // 4. 格式化日志内容（包含时间戳、类型和信息）
    QString logMsg = QString("[%1] %2: EnrollStateWidget - %3")
                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                         .arg(type)
                         .arg(message);
    qDebug() << logMsg;

    // 5. 写入日志文件（追加模式）
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << logMsg << "\n";
        file.close();
    } else {
        qWarning() << "无法打开日志文件：" << fileName << "，错误：" << file.errorString();
    }
}

// 构造函数：初始化注册状态窗口
EnrollStateWidget::EnrollStateWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EnrollStateWidget)
{
    writeEnrollStateLog("初始化登录状态窗口");
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);  // 设置无边框对话框样式
    setAttribute(Qt::WA_TranslucentBackground);            // 设置背景透明（配合自定义绘制）
    setFixedSize(325 * 0.9, 170);                          // 固定窗口大小
    setWindowModality(Qt::ApplicationModal);                // 设置为应用模态（阻塞其他窗口交互）
    writeEnrollStateLog("已设置窗口属性：无边框、透明背景、固定大小、应用级模态");
    initWidgets();                                          // 初始化界面控件
    writeEnrollStateLog("登录状态窗口初始化完成");
}

// 析构函数：释放UI资源
EnrollStateWidget::~EnrollStateWidget()
{
    writeEnrollStateLog("注册状态窗口开始销毁");
    delete ui;
    writeEnrollStateLog("注册状态窗口销毁完成");

}

// 初始化界面控件：创建标签、按钮及布局，设置样式和信号连接
void EnrollStateWidget::initWidgets()
{
    writeEnrollStateLog("开始初始化界面控件");
    // 1. 标题标签（设置对象名和样式）
    QLabel* titleLabel = new QLabel("注册失败", this);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setStyleSheet(
        "QLabel {"
        "   font-family: 'Microsoft YaHei';"
        "   font-size: 15px;"
        "   color: #333333;"
        "   font-weight: bold;"
        "}"
        );

    // 2. 提示信息标签（设置对象名和样式）
    QLabel* msgLabel = new QLabel("手机号码已被注册，请重新填写。", this);
    msgLabel->setObjectName("msgLabel");
    msgLabel->setStyleSheet(
        "QLabel {"
        "   font-family: 'Microsoft YaHei';"
        "   font-size: 14px;"
        "   color: #666666;"
        "}"
        );

    // 3. 重新注册按钮（设置对象名、样式和大小）
    QPushButton* reenrollBtn = new QPushButton("重新注册", this);
    reenrollBtn->setObjectName("reenrollBtn");
    reenrollBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: white;"
        "   color: #333333;"
        "   border: 1px solid #CCCCCC;"
        "   border-radius: 5px;"
        "   padding: 2px 15px;"
        "   font-family: 'Microsoft YaHei';"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "   border-color: #999999;"
        "}"
        );
    reenrollBtn->setFixedSize(90, 30);

    // 4. 返回登录按钮（设置对象名、样式和大小）
    QPushButton* backToloadBtn = new QPushButton("返回登录", this);
    backToloadBtn->setObjectName("backToloadBtn");  // 修正对象名
    backToloadBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: white;"
        "   color: #333333;"
        "   border: 1px solid #CCCCCC;"
        "   border-radius: 5px;"
        "   padding: 2px 15px;"
        "   font-family: 'Microsoft YaHei';"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "   border-color: #999999;"
        "}"
        );
    backToloadBtn->setFixedSize(90, 30);

    // 5. 关闭按钮（设置对象名、样式和光标）
    QPushButton* closeBtn = new QPushButton("×", this);
    closeBtn->setObjectName("closeBtn");
    closeBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: transparent;"
        "   color: #666666;"
        "   border: none;"
        "   font-size: 18px;"
        "   width: 30px;"
        "   height: 30px;"
        "   margin-top: -8px;"  // 负边距上移按钮
        "}"

        );
    closeBtn->setCursor(Qt::PointingHandCursor);  // 设置手型光标

    writeEnrollStateLog("已创建所有界面控件并设置样式");

    // 按钮布局 - 控制按钮排列和间距
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);                       // 左侧拉伸项（占比1）
    btnLayout->addWidget(reenrollBtn);              // 添加重新注册按钮
    btnLayout->addSpacing(10);                      // 按钮之间的间距
    btnLayout->addWidget(backToloadBtn);            // 添加返回登录按钮
    btnLayout->addStretch(0);                       // 右侧拉伸项（占比0）

    // 主布局 - 管理整个窗口的控件排列
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    // 创建水平布局专门放置关闭按钮
    QHBoxLayout* closeLayout = new QHBoxLayout();
    closeLayout->addStretch();                      // 左侧拉伸，将按钮挤到右侧
    closeLayout->addWidget(closeBtn);               // 添加关闭按钮
    closeLayout->addSpacing(-13);                   // 右侧减少间距，使按钮右移

    // 设置关闭按钮布局的边距（顶部负边距使按钮上移）
    closeLayout->setContentsMargins(0, -10, 0, 0);  // 上、左、下、右边距

    // 将关闭按钮布局添加到主布局
    mainLayout->addLayout(closeLayout, 0);
    mainLayout->addSpacing(10);                     // 间距
    mainLayout->addWidget(titleLabel, 0, Qt::AlignLeft | Qt::AlignTop);  // 添加标题标签（左上对齐）
    mainLayout->addWidget(msgLabel, 0, Qt::AlignLeft);                    // 添加提示标签（左对齐）
    mainLayout->addSpacing(15);                     // 间距
    mainLayout->addStretch(1);                      // 拉伸项（占比1）
    mainLayout->addLayout(btnLayout);               // 添加按钮布局
    mainLayout->addSpacing(15);                     // 间距
    mainLayout->setContentsMargins(20, 15, 15, 15); // 设置主布局的边距
    writeEnrollStateLog("界面布局设置完成");


    // 连接按钮信号：点击后发射对应信号并关闭窗口
    connect(reenrollBtn, &QPushButton::clicked, this, [=](){
        writeEnrollStateLog("用户点击了立即注册按钮");
        QApplication::restoreOverrideCursor(); // 主动恢复光标
        emit reenrollRequested();
        close();
    });
    connect(closeBtn, &QPushButton::clicked, this, [=](){
        writeEnrollStateLog("用户点击了关闭按钮");
        emit reenrollRequested();
        close();
    });
    connect(backToloadBtn, &QPushButton::clicked, this, [=](){
        writeEnrollStateLog("用户点击重新登录按钮");
        emit backToloadRequested();
        close();
    });
}

// 重写绘制事件：自定义窗口外观（绘制圆角白色背景）
void EnrollStateWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);  // 忽略未使用的参数
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);  // 启用抗锯齿（使圆角更平滑）

    QRectF windowRect = rect().adjusted(1, 1, -1, -1);  // 调整窗口矩形（向内缩进1px）
    int radius = 10;                                    // 圆角半径
    painter.setBrush(Qt::white);                        // 设置填充颜色（白色）
    painter.setPen(QPen(QColor(220, 220, 220), 1));     // 设置边框（浅灰色，1px宽）
    painter.drawRoundedRect(windowRect, radius, radius); // 绘制圆角矩形
}

// 重写移动事件：确保窗口始终居中于父窗口
void EnrollStateWidget::moveEvent(QMoveEvent *event)
{
    if (parentWidget()) {  // 若存在父窗口
        QPoint parentCenter = parentWidget()->geometry().center();  // 获取父窗口中心坐标
        // 移动窗口到父窗口中心
        move(parentCenter.x() - width()/2, parentCenter.y() - height()/2);
        writeEnrollStateLog("窗口已移动到父窗口中心位置");
    }
    QWidget::moveEvent(event);  // 调用父类方法
}
