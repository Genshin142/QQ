#include "load.h"
#include "ui_load.h"
#include "widget.h"
#include "enrollwindow.h"       // 注册窗口类
#include "loginstatewidget.h"   // 登录状态提示窗口类
#include "loginworker.h"        // 登录逻辑处理工作类
#include "usermanager.h"
#include <QPainter>             // 绘图相关类
#include <QRegularExpression>   // 正则表达式类
#include <QRegularExpressionValidator> // 正则表达式验证器
#include <QPushButton>          // 按钮类
#include <QLabel>               // 标签类
#include <QPainterPath>         // 绘图路径类
#include <QTimer>               // 定时器类
#include <QThread>              // 线程类
#include <windows.h>            // Windows系统API
#include <QMessageBox>          // 消息框类
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>               // 调试输出类
#include <QSettings>
#include <QUrlQuery>

// 日志工具函数：记录登录窗口相关日志
static void writeLoginWindowLog(const QString& message, const QString& type = "INFO") {
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

    // 获取当前日期并格式格式化为yyyyMMdd形式
    QString dateStr = QDateTime::currentDateTime().toString("yyyyMMdd");
    // 构建带日期的日志文件名，存储到日志文件夹下
    QString fileName = logDirPath + QString("/登录窗口窗口日志_%1.log").arg(dateStr);

    QString logMsg = QString("[%1] %2: LoginWindow - %3")
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


// 登录窗口类构造函数
load::load(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::load),
    mouse_press(false)  // 初始化鼠标按下状态为未按下
{
    writeLoginWindowLog("登录窗口开始初始化");
    ui->setupUi(this);  // 初始化UI界面
    // 加载已保存的登录信息


    this->setFocus(Qt::OtherFocusReason);  // 设置窗口获取焦点
    setFixedSize(325, 450);                // 固定窗口大小为325x450
    setWindowTitle(" ");                   // 设置窗口标题为空
    // 设置窗口为无边框但保留任务栏识别
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);  // 设置窗口背景透明

    m_networkManager = new QNetworkAccessManager(this);  // 初始化网络访问管理器
    QString phone, password;
    loadLoginInfo(phone, password);
    ui->lineEdit_phone->setText(phone);
    ui->lineEdit_password->setText(password);
    checkPhoneNumber(phone);
    // 初始化所有界面元素
    init_background_color();        // 初始化背景颜色及动画
    init_label_avatar();            // 初始化头像标签
    init_lineEdit_phone();          // 初始化手机号输入框
    init_lineEdit_password();       // 初始化密码输入框
    init_pushButton_load();         // 初始化登录按钮
    init_pushButton_close();        // 初始化关闭按钮
    init_pushButton_enroll();       // 初始化注册按钮
    init_checkupdate();             // 初始化检查更新按钮
    init_pushButton_forget_password();  // 初始化忘记密码按钮
    init_pushButton_clear();        // 初始化清除账号按钮
    init_pushButton_choose();       // 初始化选择账号按钮
    init_pushButton_clearPassword();// 初始化清除密码按钮
    init_checkBox_password();
    initUpdateManager();            // 初始化更新管理器

    // 设置焦点切换顺序：从手机号输入框切换到密码输入框
    setTabOrder(ui->lineEdit_phone, ui->lineEdit_password);

    // 连接信号槽：输入内容变化时检查输入有效性
    connect(ui->lineEdit_phone, &QLineEdit::textChanged, this, &load::checkInputValid);
    connect(ui->lineEdit_password, &QLineEdit::textChanged, this, &load::checkInputValid);

    // 初始检查输入状态，决定登录按钮是否可用
    checkInputValid();
    writeLoginWindowLog("登录窗口初始化完成");
}

// 析构函数：释放UI资源
load::~load()
{
    writeLoginWindowLog("登录窗口开始销毁");
    delete ui;
    writeLoginWindowLog("登录窗口销毁完成");
}

// 界面初始化相关方法实现
// 初始化背景颜色及渐变动画
void load::init_background_color()
{
    // 初始颜色设置
    m_color1 = QColor(255, 235, 245);  // 浅粉色
    m_color2 = QColor(220, 235, 255);  // 浅蓝色

    // 创建颜色动画1：控制渐变起始色
    m_anim1 = new QPropertyAnimation(this, "color1");
    m_anim1->setDuration(3000);        // 动画持续3秒
    m_anim1->setLoopCount(-1);         // 无限循环
    // 颜色变化路径：浅粉 -> 浅绿 -> 浅粉
    m_anim1->setKeyValueAt(0, QColor(255, 235, 245));
    m_anim1->setKeyValueAt(0.5, QColor(235, 255, 245));
    m_anim1->setKeyValueAt(1, QColor(255, 235, 245));

    // 创建颜色动画2：控制渐变结束色
    m_anim2 = new QPropertyAnimation(this, "color2");
    m_anim2->setDuration(5000);        // 动画持续5秒
    m_anim2->setLoopCount(-1);         // 无限循环
    // 颜色变化路径：浅蓝 -> 浅青 -> 浅蓝
    m_anim2->setKeyValueAt(0, QColor(220, 235, 255));
    m_anim2->setKeyValueAt(0.5, QColor(235, 245, 255));
    m_anim2->setKeyValueAt(1, QColor(220, 235, 255));

    // 启动动画
    m_anim1->start();
    m_anim2->start();
}

// 初始化头像标签
void load::init_label_avatar()
{
    // 设置头像位置：水平居中，顶部距离70px
    ui->label_avatar->move(width() / 2 - 45, 70);

    // 设置头像样式：圆形边框（半径45px）、白色边框、白色背景
    ui->label_avatar->setStyleSheet(
        "QLabel {"
        "   border-radius: 45px;"
        "   border: 2px solid #ffffff;"
        "   background-color: white;"
        "}"
        );
    ui->label_avatar->setFixedSize(90, 90);  // 固定大小90x90
    ui->label_avatar->setFocusPolicy(Qt::NoFocus);  // 不接受焦点

    // 读取 avatar.txt 获取头像路径
    QString avatarPath;
    QFile file("avatar.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        avatarPath = in.readLine().trimmed();  // 读取第一行并去除首尾空白
        file.close();
    }

    QPixmap pixmap;
    if (!avatarPath.isEmpty() && pixmap.load(avatarPath)) {
        // 加载文件中指定的头像路径成功
    } else {
        // 如果文件不存在或路径无效，加载默认头像
        pixmap.load(":/images/QQ.png");
    }

    QPixmap roundPixmap(ui->label_avatar->size());  // 创建圆形画布
    roundPixmap.fill(Qt::transparent);         // 画布透明
    QPainter painter(&roundPixmap);
    painter.setRenderHint(QPainter::Antialiasing);  // 抗锯齿
    QPainterPath path;
    path.addEllipse(roundPixmap.rect());       // 添加椭圆路径（圆形）
    painter.setClipPath(path);                 // 设置裁剪路径（只绘制圆形区域）
    // 绘制缩放后的图片到圆形区域
    painter.drawPixmap(0, 0, pixmap.scaled(
                                 roundPixmap.size(),
                                 Qt::KeepAspectRatio,    // 保持宽高比
                                 Qt::SmoothTransformation  // 平滑缩放
                                 ));
    ui->label_avatar->setPixmap(roundPixmap);  // 设置圆形头像
    ui->label_avatar->setScaledContents(true);  // 图片自适应标签大小
}

// 初始化手机号输入框
void load::init_lineEdit_phone()
{
    // 设置手机号输入框样式：白色背景、无边框、圆角10px、居中对齐、微软雅黑字体14px
    ui->lineEdit_phone->setStyleSheet(
        "QLineEdit {"
        "   background-color: white;"
        "   border-width: 0;"
        "   border-style: outset;"
        "   border-radius: 10px;"
        "   qproperty-alignment: 'AlignCenter';"
        "   font-family: 'Microsoft YaHei';"
        "   font-size: 14px;"
        "}"
        );
    ui->lineEdit_phone->setPlaceholderText("输入手机号");  // 占位提示文字
    ui->lineEdit_phone->setCursor(Qt::IBeamCursor);        // 光标样式为I型
    ui->lineEdit_phone->installEventFilter(this);          // 安装事件过滤器

    // 设置输入验证：最多11位数字（手机号规则）
    QRegularExpression regExp("\\d{0,11}");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(regExp, this);
    ui->lineEdit_phone->setValidator(validator);
    ui->lineEdit_phone->setMaxLength(11);  // 最大长度11位
}

// 初始化密码输入框
void load::init_lineEdit_password()
{
    // 设置密码输入框样式：居中对齐、白色背景、微软雅黑字体14px、圆角10px
    ui->lineEdit_password->setStyleSheet(
        "QLineEdit {"
        "   qproperty-alignment: 'AlignCenter';"
        "   background-color: white;"
        "   font-family: 'Microsoft YaHei';"
        "   font-size: 14px;"
        "   border-radius: 10px;"
        "}"
        );
    ui->lineEdit_password->setEchoMode(QLineEdit::Password);  // 密码显示模式（隐藏字符）
    ui->lineEdit_password->setCursor(Qt::IBeamCursor);        // 光标样式为I型
    ui->lineEdit_password->setPlaceholderText("输入QQ密码");   // 占位提示文字
    ui->lineEdit_password->installEventFilter(this);          // 安装事件过滤器
}

// 初始化登录按钮
void load::init_pushButton_load()
{
    // 设置登录按钮样式
    ui->pushButton_load->setStyleSheet(
        "QPushButton {"
        "   background-color: #0099ff;"  // 蓝色背景
        "   color: rgba(255, 255, 255, 200);"  // 半透明白色文字
        "   border-radius: 10px;"        // 圆角10px
        "   padding: 6px 12px;"          // 内边距
        "   font-family: 'Microsoft YaHei';"  // 字体
        "   font-size: 14px;"            // 字号
        "   border: none;"               // 无边框
        "}"
        "QPushButton:hover {"            // 鼠标悬停状态
        "   background-color: #0088ee;"  // 深一点的蓝色
        "   transform: scale(1.02);"     // 轻微放大
        "}"
        "QPushButton:pressed {"          // 鼠标按下状态
        "   background-color: #0077dd;"  // 更深的蓝色
        "   transform: scale(0.98);"     // 轻微缩小
        "}"
        "QPushButton:disabled {"         // 禁用状态
        "   background-color: #98d5fe;"  // 浅灰色
        "   color: rgba(255, 255, 255, 100);"  // 更浅的文字
        "}"
        );
    ui->pushButton_load->setEnabled(false);  // 初始禁用
    ui->pushButton_load->setFocusPolicy(Qt::NoFocus);  // 不接受焦点
    ui->pushButton_load->installEventFilter(this);      // 安装事件过滤器
}

// 初始化关闭按钮
void load::init_pushButton_close()
{
    // 设置关闭按钮样式：透明背景、无边框、悬停/按下时半透明背景、特定角圆角
    ui->pushButton_close->setStyleSheet(
        "QPushButton {"
        "   border: none;"
        "   background-color: transparent;"
        "   padding: 5px 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(200, 200, 200, 50);"
        "   border-top-right-radius: 10px;"
        "   border-bottom-left-radius: 10px;"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(150, 150, 150, 80);"
        "   border-top-right-radius: 10px;"
        "   border-bottom-left-radius: 10px;"
        "}"
        );
    ui->pushButton_close->setFocusPolicy(Qt::NoFocus);  // 不接受焦点
}

// 初始化注册按钮
void load::init_pushButton_enroll()
{
    // 设置注册按钮样式：透明背景、无边框、蓝色文字、悬停时颜色变深
    ui->pushButton_enroll->setStyleSheet(
        "QPushButton {"
        "   border: none;"
        "   background-color: transparent;"
        "   color: #0066cc;"
        "}"
        "QPushButton:hover {"
        "   color: #0099ff;"
        "}"
        );
    ui->pushButton_enroll->setCursor(Qt::ArrowCursor);  // 光标样式
    ui->pushButton_enroll->setAttribute(Qt::WA_Hover, true);  // 启用悬停事件
    ui->pushButton_enroll->setFocusPolicy(Qt::NoFocus);       // 不接受焦点
    ui->pushButton_enroll->installEventFilter(this);           // 安装事件过滤器
}

void load::init_checkupdate()
{

        // 设置注册按钮样式：透明背景、无边框、蓝色文字、悬停时颜色变深
        ui->checkUpdateButton->setStyleSheet(
            "QPushButton {"
            "   border: none;"
            "   background-color: transparent;"
            "   color: #0066cc;"
            "}"
            "QPushButton:hover {"
            "   color: #0099ff;"
            "}"
            );
        ui->checkUpdateButton->setCursor(Qt::ArrowCursor);  // 光标样式
        ui->checkUpdateButton->setAttribute(Qt::WA_Hover, true);  // 启用悬停事件
        ui->checkUpdateButton->setFocusPolicy(Qt::NoFocus);       // 不接受焦点
        ui->checkUpdateButton->installEventFilter(this);           // 安装事件过滤器

}

// 初始化忘记密码按钮
void load::init_pushButton_forget_password()
{
    // 设置忘记密码按钮样式：同注册按钮
    ui->pushButton_forget_password->setStyleSheet(
        "QPushButton {"
        "   border: none;"
        "   background-color: transparent;"
        "   color: #0066cc;"
        "}"
        "QPushButton:hover {"
        "   color: #0099ff;"
        "}"
        );
    ui->pushButton_forget_password->setCursor(Qt::ArrowCursor);  // 光标样式
    ui->pushButton_forget_password->setAttribute(Qt::WA_Hover, true);  // 启用悬停事件
    ui->pushButton_forget_password->setFocusPolicy(Qt::NoFocus);       // 不接受焦点
    ui->pushButton_forget_password->installEventFilter(this);           // 安装事件过滤器
}

// 初始化清除账号按钮
void load::init_pushButton_clear()
{
    // 设置清除账号按钮样式：透明背景、无边框
    ui->pushButton_clear->setStyleSheet("background:transparent;border-width:0;border-style:outset");
    ui->pushButton_clear->setEnabled(true);  // 启用按钮
    ui->pushButton_clear->setFocusPolicy(Qt::NoFocus);  // 不接受焦点

    // 调整位置：向上移动3px
    QPoint clearPos = ui->pushButton_clear->pos();
    ui->pushButton_clear->move(clearPos.x(), clearPos.y() - 3);
    ui->pushButton_clear->raise();  // 提升层级（显示在输入框上方）
    ui->pushButton_clear->hide();   // 初始隐藏
}

// 初始化选择账号按钮
void load::init_pushButton_choose()
{
    // 设置选择账号按钮样式：透明背景、无边框
    ui->pushButton_choose->setStyleSheet("background:transparent;border-width:0;border-style:outset");
    ui->pushButton_choose->setFocusPolicy(Qt::NoFocus);  // 不接受焦点

    // 调整位置：向上移动3px
    QPoint choosePos = ui->pushButton_choose->pos();
    ui->pushButton_choose->move(choosePos.x(), choosePos.y() - 3);
    ui->pushButton_choose->raise();  // 提升层级
}

// 初始化清除密码按钮
void load::init_pushButton_clearPassword()
{
    // 设置清除密码按钮样式：透明背景、无边框
    ui->pushButton_clearPassword->setStyleSheet("background:transparent;border-width:0;border-style:outset");
    ui->pushButton_clearPassword->setFocusPolicy(Qt::NoFocus);  // 不接受焦点

    // 调整位置：向上移动3px
    QPoint clearPasswordPos = ui->pushButton_clearPassword->pos();
    ui->pushButton_clearPassword->move(clearPasswordPos.x(), clearPasswordPos.y() - 3);
    ui->pushButton_clearPassword->hide();  // 初始隐藏
}

// 初始化记住密码复选框（样式+状态恢复）
void load::init_checkBox_password() {

    // 从配置文件读取并恢复勾选状态（关键逻辑）
    QString configPath = QCoreApplication::applicationDirPath() + "/login_config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    bool isChecked = settings.value("LoginInfo/save", false).toBool();
    ui->checkBox_password->setChecked(isChecked);  // 恢复上次勾选状态
}

// 事件处理实现
// 重绘事件：绘制窗口背景
void load::paintEvent(QPaintEvent *event)
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

// 鼠标移动事件：实现无边框窗口拖动
void load::mouseMoveEvent(QMouseEvent *event)
{
    if (mouse_press) {  // 如果鼠标已按下
        // 移动窗口到当前鼠标位置减去初始点击偏移量
        move(event->globalPos() - mousePoint);
    }
}

// 鼠标按下事件：记录拖动起始位置
void load::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 获取点击位置的子控件
        QWidget *child = childAt(event->pos());

        // 判断点击的是否是账号或密码输入框
        bool isPhoneEdit = (child == ui->lineEdit_phone);
        bool isPwdEdit = (child == ui->lineEdit_password);

        if (!child) { // 点击空白区域
            mouse_press = true;
            mousePoint = event->globalPos() - this->pos();
            // 清除输入框焦点
            ui->lineEdit_phone->clearFocus();
            ui->lineEdit_password->clearFocus();
            setFocus(); // 窗口自身获取焦点
        } else if (!isPhoneEdit && !isPwdEdit) {
            // 点击了其他控件（非账号和密码输入框）
            ui->lineEdit_phone->clearFocus();
            ui->lineEdit_password->clearFocus();
            setFocus(); // 窗口自身获取焦点
        }
        // 点击账号/密码输入框时不处理（保留默认焦点行为）
    } else if (event->button() == Qt::RightButton) {
        this->close();  // 右键点击关闭窗口
    }
}

// 鼠标释放事件：结束拖动状态
void load::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    mouse_press = false;  // 重置鼠标按下状态
}

// 键盘按下事件：处理回车键登录
void load::keyPressEvent(QKeyEvent *event)
{
    // 处理回车键登录
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (ui->pushButton_load->isEnabled()) {
            on_pushButton_load_clicked();  // 触发登录
            writeLoginWindowLog("用户按下回车键触发登录");
        }
    }
}

// 事件过滤器：处理各种控件的事件
bool load::eventFilter(QObject *watched, QEvent *event)
{
    // 处理手机号输入框焦点事件
    if (watched == ui->lineEdit_phone) {
        if (event->type() == QEvent::FocusIn) {
            // 获得焦点时，显示清除按钮
            ui->pushButton_clear->setVisible(true);
            ui->pushButton_clearPassword->hide(); // 隐藏密码框的清除按钮
        } else if (event->type() == QEvent::FocusOut) {
            // 失去焦点时延迟隐藏清除按钮
            QTimer::singleShot(100, [=](){
                ui->pushButton_clear->hide();
            });
        }
    }
    // 处理密码输入框焦点事件
    else if (watched == ui->lineEdit_password) {
        if (event->type() == QEvent::FocusIn) {
            // 获得焦点时，显示清除按钮
            ui->pushButton_clearPassword->setVisible(true);
            ui->pushButton_clear->hide(); // 隐藏手机号框的清除按钮
        } else if (event->type() == QEvent::FocusOut) {
            // 失去焦点时延迟隐藏清除按钮
            QTimer::singleShot(100, [=](){
                ui->pushButton_clearPassword->hide();
            });
        }
    }
    // 处理"注册"按钮
    if (watched == ui->pushButton_enroll) {
        if (event->type() == QEvent::Enter) {
            // 鼠标进入按钮区域，切换为食指光标
            ui->pushButton_enroll->setCursor(Qt::PointingHandCursor);
            return true; // 事件已处理
        } else if (event->type() == QEvent::Leave) {
            // 鼠标离开按钮区域，恢复默认光标
            ui->pushButton_enroll->setCursor(Qt::ArrowCursor);
            return true; // 事件已处理
        }
    }
    // 处理"忘记密码"按钮
    else if (watched == ui->pushButton_forget_password) {
        if (event->type() == QEvent::Enter) {
            ui->pushButton_forget_password->setCursor(Qt::PointingHandCursor);
            return true;
        } else if (event->type() == QEvent::Leave) {
            ui->pushButton_forget_password->setCursor(Qt::ArrowCursor);
            return true;
        }
    }
    else if (watched == ui->checkUpdateButton) {
        if (event->type() == QEvent::Enter) {
            ui->checkUpdateButton->setCursor(Qt::PointingHandCursor);
            return true;
        } else if (event->type() == QEvent::Leave) {
            ui->checkUpdateButton->setCursor(Qt::ArrowCursor);
            return true;
        }
    }

    // 处理登录按钮的鼠标进入/离开事件
    if (watched == ui->pushButton_load) {
        bool isDisabled = !ui->pushButton_load->isEnabled(); // 获取按钮禁用状态
        if (event->type() == QEvent::Enter) {
            // 鼠标进入按钮：根据状态设置全局光标
            QApplication::setOverrideCursor(isDisabled ? Qt::ForbiddenCursor : Qt::ArrowCursor);
            return true;
        } else if (event->type() == QEvent::Leave) {
            // 鼠标离开按钮：恢复全局光标
            QApplication::restoreOverrideCursor();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

// 关闭按钮点击事件
void load::on_pushButton_close_clicked()
{
    writeLoginWindowLog("用户点击关闭按钮");
    this->close();  // 关闭窗口
}

// 注册按钮点击事件
void load::on_pushButton_enroll_clicked()
{
    writeLoginWindowLog("用户点击注册按钮，跳转到注册窗口");
    EnrollWindow *enrollWindow = new EnrollWindow();
    // 连接注册窗口的返回信号到当前登录窗口
    connect(enrollWindow, &EnrollWindow::backToLoginRequested, this, [=](){
        writeLoginWindowLog("从注册窗口返回登录窗口");
        this->setFocus();  // 当前窗口焦点
        ui->lineEdit_password->clearFocus();
        ui->lineEdit_phone->clearFocus();
        this->show();
        this->raise();
        this->activateWindow();
    });

    enrollWindow->show();  // 显示注册窗口

}

// 登录按钮点击事件
void load::on_pushButton_load_clicked()
{
    writeLoginWindowLog("用户点击登录按钮，开始登录流程");  // 添加登录按钮日志
    QString account = ui->lineEdit_phone->text().trimmed();  // 获取账号（去除空格）
    QString password = ui->lineEdit_password->text().trimmed();  // 获取密码（去除空格）
    saveLoginInfo(account, password);  // 保存登录信息

    // 创建工作线程和工作对象（避免登录过程阻塞UI）
    QThread* workerThread = new QThread(this);
    LoginWorker* worker = new LoginWorker(account, password);

    // 将工作对象移动到子线程
    worker->moveToThread(workerThread);

    // 连接信号槽（工作线程 -> 主线程）
    connect(worker, &LoginWorker::loginResult, this, &load::onLoginResult);
    connect(worker, &LoginWorker::errorOccurred, this, &load::onErrorOccurred);
    // 连接线程启动信号
    connect(workerThread, &QThread::started, worker, &LoginWorker::doLogin);
    // 线程结束后清理
    connect(worker, &LoginWorker::finished, workerThread, &QThread::quit);
    connect(worker, &LoginWorker::finished, worker, &LoginWorker::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);

    // 启动线程执行登录逻辑
    workerThread->start();
}

// 处理登录结果
void load::onLoginResult(bool success, const QString& message, const QString& userName,const QString& userId,const QString& phone)
{
    if (success) {

        writeLoginWindowLog(QString("登录成功，用户名: %1").arg(userName));  // 添加登录成功日志
        // 登录成功时设置用户ID
        UserManager::getInstance().setUserId(userId);
        UserManager::getInstance().setPhoneNumber(phone);
        UserManager::getInstance().setName(userName);      // 设置用户名
        // 登录成功，创建并显示主窗口
        Widget* mainWidget = new Widget();
        mainWidget->show();
        this->close();  // 登录成功，关闭登录窗口

    }  else {
        writeLoginWindowLog(QString("登录失败: %1").arg(message), "ERROR");  // 添加登录失败日志
        // 登录失败，显示错误提示窗口
        LoginStateWidget *errorWidget = new LoginStateWidget();
        if (message == "该账号已登录") {
            // 通过对象名查找子控件（需确保msgLabel设置了objectName）
            QLabel* msgLabel = errorWidget->findChild<QLabel*>("msgLabel");
            if (msgLabel) {
                msgLabel->setText("该账号已登录"); // 直接修改文本
                writeLoginWindowLog("已更新提示信息为：该账号已登录");
            } else {
                writeLoginWindowLog("未找到msgLabel控件", "WARNING");
            }
        }

        // 连接重新登录信号
        connect(errorWidget, &LoginStateWidget::reloginRequested, this, [=](){
            // 重置登录窗口状态
            ui->lineEdit_password->clear(); // 清空密码输入框
            ui->lineEdit_phone->clearFocus();
            ui->lineEdit_password->clearFocus();
            this->setFocus();
            this->raise(); // 提升登录窗口层级
            this->activateWindow(); // 激活登录窗口
        });

        // 计算错误窗口位置（居中于当前登录窗口）
        QPoint loadCenter = this->geometry().center();
        QRect errorRect = errorWidget->geometry();
        errorWidget->move(
            loadCenter.x() - errorRect.width()/2,
            loadCenter.y() - errorRect.height()/2
            );
        errorWidget->setWindowFlags(errorWidget->windowFlags() | Qt::WindowStaysOnTopHint);
        errorWidget->show();
        errorWidget->raise();
        errorWidget->activateWindow();
        errorWidget->setAttribute(Qt::WA_DeleteOnClose);  // 关闭时自动释放
    }
}

// 检查输入有效性，更新登录按钮状态
void load::checkInputValid()
{
    // 获取账号和密码输入内容（去除空格）
    QString account = ui->lineEdit_phone->text().trimmed();
    QString password = ui->lineEdit_password->text().trimmed();

    // 判断输入是否有效（非空）
    bool isValid = !account.isEmpty() && !password.isEmpty();

    // 更新按钮状态
    ui->pushButton_load->setEnabled(isValid);

    // 设置光标样式
    if (isValid) {
        ui->pushButton_load->setCursor(Qt::ArrowCursor);
    } else {
        ui->pushButton_load->setCursor(Qt::ForbiddenCursor);
    }
    // 若鼠标正悬停在登录按钮上，强制更新光标
    if (ui->pushButton_load->underMouse()) {
        QApplication::setOverrideCursor(isValid ? Qt::ArrowCursor : Qt::ForbiddenCursor);
    }

    // 根据手机号长度处理头像
    if (account.length() == 11) {
        // 11位手机号，调用checkPhoneNumber获取对应头像
        checkPhoneNumber(account);
    } else {
        // 非11位，显示默认头像，保持与init_label_avatar相同的样式
        downloadDefaultAvatar();
    }
}

// 处理登录过程中的错误
void load::onErrorOccurred(const QString& errorMsg)
{
    writeLoginWindowLog(QString("登录过程发生错误: %1").arg(errorMsg), "ERROR");  // 添加错误日志
    QMessageBox::critical(this, "错误", errorMsg);  // 显示错误消息框
}

// 清除账号按钮点击事件
void load::on_pushButton_clear_clicked()
{
    writeLoginWindowLog("用户点击清除账号按钮");  // 添加登录按钮日志
    ui->lineEdit_phone->clear();  // 清空手机号输入框
    ui->lineEdit_phone->setFocus(); // 清除后重新获取焦点
    ui->pushButton_clear->hide();  // 手动隐藏按钮
    clearSavedPassword();  // 清除保存的密码
    writeLoginWindowLog("已清除账号输入和保存的密码");
}

// 清除密码按钮点击事件
void load::on_pushButton_clearPassword_clicked()
{
    writeLoginWindowLog("用户点击清除密码按钮");  // 添加登录按钮日志
    ui->lineEdit_password->clear();  // 清空密码输入框
    ui->lineEdit_password->setFocus(); // 清除后重新获取焦点
    ui->pushButton_clearPassword->hide();  // 手动隐藏按钮
}
// 保存登录信息到本地配置文件
void load::saveLoginInfo(const QString& phone, const QString& password) {
    QByteArray encryptedPhone = phone.toUtf8().toBase64();
    QByteArray encryptedPwd = password.toUtf8().toBase64();

    QString configPath = QCoreApplication::applicationDirPath() + "/login_config.ini";
    QSettings settings(configPath, QSettings::IniFormat);

    // 始终保存手机号（无论是否勾选，都保存手机号）
    settings.setValue("LoginInfo/phone", encryptedPhone);
    // 只有勾选记住密码时才保存密码
    if (ui->checkBox_password->isChecked()) {
        settings.setValue("LoginInfo/password", encryptedPwd);
    } else {
        // 未勾选时清除已保存的密码
        settings.remove("LoginInfo/password");
    }
    // 保存勾选状态
    settings.setValue("LoginInfo/save", ui->checkBox_password->isChecked());

    writeLoginWindowLog("已保存登录信息（手机号始终保存）");
}

// 加载登录信息
void load::loadLoginInfo(QString& phone, QString& password) {
    QString configPath = QCoreApplication::applicationDirPath() + "/login_config.ini";
    QSettings settings(configPath, QSettings::IniFormat);

    // 始终加载手机号（无论是否勾选记住密码）
    QByteArray encryptedPhone = settings.value("LoginInfo/phone").toByteArray();
    phone = QByteArray::fromBase64(encryptedPhone);

    // 只有勾选状态为true时才加载密码
    if (settings.value("LoginInfo/save", false).toBool()) {
        QByteArray encryptedPwd = settings.value("LoginInfo/password").toByteArray();
        password = QByteArray::fromBase64(encryptedPwd);
    } else {
        password.clear(); // 未勾选时密码为空
    }

    writeLoginWindowLog("已加载保存的登录信息（未勾选时仅加载手机号）");
}

// 清除保存的密码（保留手机号）
void load::clearSavedPassword() {
    QString configPath = QCoreApplication::applicationDirPath() + "/login_config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    settings.remove("LoginInfo/password");
    settings.setValue("LoginInfo/save", false);
}

void load::checkPhoneNumber(const QString &phoneNumber)
{
    writeLoginWindowLog(QString("开始检查手机号并获取头像: %1").arg(phoneNumber), "INFO");

    // 构建请求URL
    QUrl url("http://101.37.68.162:8080/api/check_phone");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 构建请求数据
    QJsonObject json;
    json["phone"] = phoneNumber;
    QByteArray postData = QJsonDocument(json).toJson();

    writeLoginWindowLog(QString("发送手机号检查请求到: %1").arg(url.toString()), "INFO");

    // 发送POST请求
    QNetworkReply* reply = m_networkManager->post(request, postData);

    // 设置超时处理（5秒）
    QTimer::singleShot(5000, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
            qWarning() << "手机号检查请求超时";
        }
    });

    // 处理响应
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            writeLoginWindowLog(QString("收到手机号检查响应，数据大小: %1 字节").arg(responseData.size()), "INFO");

            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
            if (doc.isNull()) {
                writeLoginWindowLog(QString("解析手机号检查响应失败: %1（偏移量: %2）")
                                        .arg(parseError.errorString())
                                        .arg(parseError.offset), "ERROR");
            } else {
                QJsonObject response = doc.object();
                if (response["success"].toBool()) {
                    QString user_id = response["id"].toString();
                    writeLoginWindowLog(QString("用户id: %1").arg(user_id), "INFO");

                    // 下载并设置头像
                    downloadAndSetAvatar(user_id);
                } else {
                    downloadDefaultAvatar();
                    writeLoginWindowLog(QString("手机号检查失败: %1")
                                            .arg(response["error"].toString()), "ERROR");
                }
            }
        } else {
            writeLoginWindowLog(QString("手机号检查网络错误: %1").arg(reply->errorString()), "ERROR");
        }
        reply->deleteLater();
    });
}
void load::downloadDefaultAvatar()
{
    writeLoginWindowLog("开始加载默认头像", "INFO");

    // 加载本地默认头像资源
    QPixmap pixmap(":/images/QQ.png");
    if (pixmap.isNull()) {
        writeLoginWindowLog("无法加载默认头像资源 :/images/QQ.png", "ERROR");
        return;
    }

    // 与网络下载头像保持一致的圆形处理
    QPixmap roundPixmap(ui->label_avatar->size());
    roundPixmap.fill(Qt::transparent);
    QPainter painter(&roundPixmap);
    painter.setRenderHint(QPainter::Antialiasing);  // 抗锯齿
    QPainterPath path;
    path.addEllipse(roundPixmap.rect());  // 创建圆形裁剪路径
    painter.setClipPath(path);  // 设置裁剪路径

    // 绘制并缩放头像，保持与网络下载头像相同的处理方式
    painter.drawPixmap(0, 0, pixmap.scaled(
                                 roundPixmap.size(),
                                 Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation  // 平滑缩放
                                 ));

    // 设置头像到标签
    ui->label_avatar->setPixmap(roundPixmap);
    writeLoginWindowLog("默认头像加载并设置完成", "INFO");
}

void load::initUpdateManager()
{
    // 初始化更新管理器
    m_updateManager = new UpdateManager(this);
    // 设置当前版本 - 实际应用中可以从配置文件或编译信息中获取
    m_updateManager->setCurrentVersion("1.0.0");
    // 设置更新服务器地址
    m_updateManager->setUpdateServerUrl("https://your-server.com/updates/version.json");

    // 连接更新管理器信号
    connect(m_updateManager, &UpdateManager::updateCheckFinished,
            this, &load::onUpdateCheckFinished);
    connect(m_updateManager, &UpdateManager::downloadProgress,
            this, &load::onDownloadProgress);
    connect(m_updateManager, &UpdateManager::downloadFinished,
            this, &load::onDownloadFinished);
    connect(m_updateManager, &UpdateManager::errorOccurred,
            this, &load::onUpdateError);

    // 检查更新按钮
    connect(ui->checkUpdateButton, &QPushButton::clicked,
            m_updateManager, &UpdateManager::checkForUpdates);
}
void load::onUpdateCheckFinished(bool hasUpdate, const QString &latestVersion, const QString &releaseNotes)
{
    if (hasUpdate) {
        QMessageBox::StandardButton reply;
        QString message = QString("发现新版本 %1，是否更新？\n\n更新内容：\n%2")
                              .arg(latestVersion).arg(releaseNotes);
        reply = QMessageBox::question(this, "更新提示", message,
                                      QMessageBox::Yes|QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            // 显示下载进度对话框
            m_progressDialog = new QProgressDialog("正在下载更新...", "取消", 0, 100, this);
            m_progressDialog->setWindowTitle("下载更新");
            m_progressDialog->setWindowModality(Qt::WindowModal);
            m_updateManager->downloadUpdate();
        }
    } else {
        QMessageBox::information(this, "更新检查", "当前已是最新版本");
    }
}

void load::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (m_progressDialog && bytesTotal > 0) {
        int percentage = (bytesReceived * 100) / bytesTotal;
        m_progressDialog->setValue(percentage);
    }
}

void load::onDownloadFinished(const QString &filePath)
{
    if (m_progressDialog) {
        m_progressDialog->close();
        delete m_progressDialog;
        m_progressDialog = nullptr;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "下载完成",
                                  "更新包已下载完成，是否立即安装并重启应用？",
                                  QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_updateManager->installUpdate();
    }
}

void load::onUpdateError(const QString &errorMessage)
{
    QMessageBox::critical(this, "更新错误", errorMessage);
}
// 辅助函数：下载并设置头像
void load::downloadAndSetAvatar(const QString &user_id)
{
    writeLoginWindowLog(QString("准备下载头像，用户ID: %1").arg(user_id), "INFO");
    // 修正：仅通过API获取头像，使用正确的接口和参数
    QUrl url("http://101.37.68.162:8080/api/get_avatar");
    QUrlQuery query;
    // 假设avatarPath中存储的是用户ID（如"0001"）
    query.addQueryItem("user_id", user_id);
    url.setQuery(query);

    // 日志输出，与示例中的正确URL格式保持一致
    // 修正后的代码
    writeLoginWindowLog(QString("构建头像请求URL: %1").arg(url.toString()), "INFO");


    writeLoginWindowLog(QString("开始下载头像: %1").arg(url.toString()), "INFO");

    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray imageData = reply->readAll();
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
                writeLoginWindowLog("头像下载成功并设置完成", "INFO");
            } else {
                writeLoginWindowLog("无法解析头像数据为图片", "ERROR");
            }
        } else {
            writeLoginWindowLog(QString("头像下载失败: %1").arg(reply->errorString()), "ERROR");
        }
        reply->deleteLater();
    });
}
