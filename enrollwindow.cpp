#include "enrollwindow.h"
#include "ui_enrollwindow.h"
#include "enrollstatewidget.h"
#include "enrollworker.h"
#include <QThread>
#include <QMessageBox>
#include <QPainter>
#include <QPropertyAnimation>
#include <QApplication>
#include <QMouseEvent>
#include <QStyle>
#include <QFile>
#include <QDir>
// 日志工具函数：记录注册窗口相关日志
static void writeEnrollWindowLog(const QString& message, const QString& type = "INFO") {
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
    QString fileName = logDirPath + QString("/注册窗口日志_%1.log").arg(dateStr);

    QString logMsg = QString("[%1] %2: EnrollWindow - %3")
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


// 构造函数：初始化注册窗口，设置基本属性和控件
EnrollWindow::EnrollWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EnrollWindow),
    mouse_press(false)  // 初始化鼠标按下状态为未按下
{
    // 记录窗口初始化开始日志
    writeEnrollWindowLog("注册窗口初始化开始");
    ui->setupUi(this);
    this->setFocus(Qt::OtherFocusReason);  // 设置窗口获取焦点
    setFixedSize(600, 600);                // 固定窗口大小
    setWindowTitle("注册");                // 设置窗口标题
    init_background_color();               // 初始化背景渐变动画
    init_label_welcome();                  // 初始化欢迎标签
    init_lineEdit_name();                  // 初始化姓名输入框
    init_lineEdit_enroll_password();       // 初始化密码输入框
    init_lineEdit_phonenumber();           // 初始化手机号输入框
    init_pushbutton_enroll();              // 初始化注册按钮
    init_pushbutton_show();                // 初始化密码显示/隐藏按钮
    init_groupbox();                       // 初始化密码输入框容器

    // 设置焦点切换顺序（Tab键导航）
    setTabOrder(ui->lineEdit_name, ui->lineEdit_enroll_password);
    setTabOrder(ui->lineEdit_enroll_password, ui->lineEdit_phonenumber);
    setTabOrder(ui->lineEdit_phonenumber, ui->lineEdit_name);

    // 禁用部分控件的焦点（避免Tab键选中）
    ui->pushButton_enroll->setFocusPolicy(Qt::NoFocus);
    ui->pushButton_show_password->setFocusPolicy(Qt::NoFocus);
    ui->label_welcome->setFocusPolicy(Qt::NoFocus);
    ui->groupBox->setFocusPolicy(Qt::NoFocus);
    if (findChild<QLabel*>("label_name_tip")) findChild<QLabel*>("label_name_tip")->setFocusPolicy(Qt::NoFocus);
    if (findChild<QLabel*>("label_phonenumber_tip")) findChild<QLabel*>("label_phonenumber_tip")->setFocusPolicy(Qt::NoFocus);
    if (findChild<QLabel*>("label_pwd_tip_0")) findChild<QLabel*>("label_pwd_tip_0")->setFocusPolicy(Qt::NoFocus);
    if (findChild<QLabel*>("label_pwd_tip_1")) findChild<QLabel*>("label_pwd_tip_1")->setFocusPolicy(Qt::NoFocus);

    // 连接输入框文本变化信号到验证函数（实时检查输入有效性）
    connect(ui->lineEdit_name, &QLineEdit::textChanged, this, &EnrollWindow::checkAllValid);
    connect(ui->lineEdit_enroll_password, &QLineEdit::textChanged, this, &EnrollWindow::checkAllValid);
    connect(ui->lineEdit_phonenumber, &QLineEdit::textChanged, this, &EnrollWindow::checkAllValid);

    checkAllValid();  // 初始检查输入状态，设置注册按钮可用性
    // 记录窗口初始化完成日志
    writeEnrollWindowLog("注册窗口初始化完成");
}

// 析构函数：释放UI资源
EnrollWindow::~EnrollWindow()
{
    delete ui;
}

// 初始化背景渐变动画（设置颜色和动画参数）
void EnrollWindow::init_background_color()
{
    m_color1 = QColor(255, 235, 245);  // 初始背景色1
    m_color2 = QColor(220, 235, 255);  // 初始背景色2

    // 初始化颜色1的动画（3秒循环渐变）
    m_anim1 = new QPropertyAnimation(this, "color1");
    m_anim1->setDuration(3000);
    m_anim1->setLoopCount(-1);  // 无限循环
    m_anim1->setKeyValueAt(0, QColor(255, 235, 245));
    m_anim1->setKeyValueAt(0.5, QColor(235, 255, 245));
    m_anim1->setKeyValueAt(1, QColor(255, 235, 245));

    // 初始化颜色2的动画（5秒循环渐变）
    m_anim2 = new QPropertyAnimation(this, "color2");
    m_anim2->setDuration(5000);
    m_anim2->setLoopCount(-1);  // 无限循环
    m_anim2->setKeyValueAt(0, QColor(220, 235, 255));
    m_anim2->setKeyValueAt(0.5, QColor(235, 245, 255));
    m_anim2->setKeyValueAt(1, QColor(220, 235, 255));

    // 启动动画
    m_anim1->start();
    m_anim2->start();
}

// 初始化欢迎标签（设置样式和文本）
void EnrollWindow::init_label_welcome()
{
    ui->label_welcome->setStyleSheet(
        "QLabel {"
        "   border: none !important;"
        "   font-family: 'Microsoft YaHei' !important;"
        "   font-size: 26px !important;"
        "   background-color: transparent !important;"
        "   color: #333333 !important;"
        "   text-align: center !important;"
        "}"
        );
    ui->label_welcome->setText("欢迎注册QQ");  // 设置标签文本
    ui->label_welcome->setFixedWidth(300);     // 固定宽度
    ui->label_welcome->setFixedHeight(50);     // 固定高度
}

// 初始化姓名输入框（设置样式、占位符和验证提示）
void EnrollWindow::init_lineEdit_name()
{
    ui->lineEdit_name->setStyleSheet(
        "QLineEdit {"
        "   qproperty-alignment: 'AlignLeft';"
        "   padding-left: 10px;"
        "   background-color: white;"
        "   font-family: 'Microsoft YaHei';"
        "   font-size: 14px;"
        "   border-radius: 10px;"
        "   border: 1px solid #CCCCCC;"
        "}"
        "QLineEdit:focus {"
        "   border: 1px solid #0099ff;"  // 获焦时边框变蓝
        "}"
        );
    ui->lineEdit_name->setPlaceholderText("输入昵称");  // 设置占位文本
    ui->lineEdit_name->setCursor(Qt::IBeamCursor);      // 设置光标样式
    ui->lineEdit_name->installEventFilter(this);        // 安装事件过滤器

    // 创建姓名验证提示标签（默认隐藏）
    QLabel *label_name_tip = new QLabel("昵称不可以为空", this);
    label_name_tip->setObjectName("label_name_tip");
    label_name_tip->setStyleSheet("color: #ff0000; font-size: 12px;");  // 红色提示
    label_name_tip->setVisible(false);
    // 设置提示标签位置（输入框下方）
    int x = ui->lineEdit_name->x();
    int y = ui->lineEdit_name->y() + ui->lineEdit_name->height() + 2;
    label_name_tip->setGeometry(x, y, 150, 20);
}

// 初始化手机号输入框（设置样式、验证和提示）
void EnrollWindow::init_lineEdit_phonenumber()
{
    ui->lineEdit_phonenumber->setStyleSheet(
        "QLineEdit {"
        "   qproperty-alignment: 'AlignLeft';"
        "   padding-left: 10px;"
        "   background-color: white;"
        "   font-family: 'Microsoft YaHei';"
        "   font-size: 14px;"
        "   border-radius: 10px;"
        "   border: 1px solid #CCCCCC;"
        "}"
        "QLineEdit:focus {"
        "   border: 1px solid #0099ff;"  // 获焦时边框变蓝
        "}"
        );
    ui->lineEdit_phonenumber->setPlaceholderText("输入手机号码");  // 占位文本
    ui->lineEdit_phonenumber->setCursor(Qt::IBeamCursor);          // 光标样式
    ui->lineEdit_phonenumber->installEventFilter(this);            // 事件过滤器
    ui->lineEdit_phonenumber->setMaxLength(11);                    // 限制11位（手机号长度）

    // 创建手机号验证提示标签（默认隐藏）
    QLabel *label_phonenumber_tip = new QLabel("手机号码不能为空", this);
    label_phonenumber_tip->setObjectName("label_phonenumber_tip");
    label_phonenumber_tip->setStyleSheet("color: #ff0000; font-size: 12px;");  // 红色提示
    label_phonenumber_tip->setVisible(false);
    // 设置提示标签位置（输入框下方）
    int x = ui->lineEdit_phonenumber->x();
    int y = ui->lineEdit_phonenumber->y() + ui->lineEdit_phonenumber->height() + 2;
    label_phonenumber_tip->setGeometry(x, y, 150, 20);
    // 微调输入框位置（上移2px）
    QPoint currentPos = ui->lineEdit_phonenumber->pos();
    ui->lineEdit_phonenumber->move(currentPos.x(), currentPos.y() - 2);
}

// 初始化密码输入框的容器（groupBox样式）
void EnrollWindow::init_groupbox()
{
    ui->groupBox->setStyleSheet("QGroupBox {"
                                "background-color: white;"
                                "border: 1px solid #CCCCCC;"
                                "border-radius: 10px;"
                                "margin-top: 10px;"
                                "}"
                                "}");
}

// 初始化密码显示/隐藏按钮（设置图标和位置）
void EnrollWindow::init_pushbutton_show()
{
    QPixmap pix(":/images/hide_password.png");  // 初始图标（隐藏密码）
    ui->pushButton_show_password->setIcon(pix);
    ui->pushButton_show_password->setIconSize(QSize(20, 20));  // 图标大小
    ui->pushButton_show_password->setStyleSheet("border: none; background-color: transparent;");  // 透明样式
    ui->pushButton_show_password->setParent(ui->groupBox);  // 设置父容器为groupBox

    // 计算按钮位置（groupBox右侧）
    int groupBoxWidth = ui->groupBox->width();
    int groupBoxHeight = ui->groupBox->height();
    int buttonWidth = 24;
    int buttonHeight = 24;
    int rightMargin = 10;
    int x = groupBoxWidth - buttonWidth - rightMargin;
    int y = (groupBoxHeight - buttonHeight) / 2;
    ui->pushButton_show_password->setGeometry(x, y+5, buttonWidth, buttonHeight);  // 定位按钮
}

// 初始化密码输入框（设置样式、密码模式和验证提示）
void EnrollWindow::init_lineEdit_enroll_password()
{
    ui->lineEdit_enroll_password->setParent(ui->groupBox);  // 设置父容器为groupBox
    ui->lineEdit_enroll_password->setGeometry(0, 13, 221, 36);  // 定位输入框
    ui->lineEdit_enroll_password->setStyleSheet(
        "QLineEdit {"
        "   qproperty-alignment: 'AlignLeft';"
        "   padding-left: 10px;"
        "   background-color: transparent;"  // 透明背景（依赖groupBox的白色背景）
        "   font-family: 'Microsoft YaHei';"
        "   font-size: 14px;"
        "   border-radius: 10px;"
        "}"
        );
    ui->lineEdit_enroll_password->setEchoMode(QLineEdit::Password);  // 密码模式（隐藏输入）
    ui->lineEdit_enroll_password->setCursor(Qt::IBeamCursor);        // 光标样式
    ui->lineEdit_enroll_password->setPlaceholderText("输入密码（8-16位）");  // 占位文本
    ui->lineEdit_enroll_password->setMaxLength(16);                  // 最大长度16位
    ui->lineEdit_enroll_password->installEventFilter(this);          // 事件过滤器

    // 创建密码验证提示标签（默认隐藏）
    QLabel *label_pwd_tip_0 = new QLabel("密码不能为空", this);
    label_pwd_tip_0->setObjectName("label_pwd_tip_0");
    label_pwd_tip_0->setStyleSheet("color: #ff0000; font-size: 12px;");  // 红色提示
    label_pwd_tip_0->setVisible(false);

    QLabel *label_pwd_tip_1 = new QLabel("密码为8-16位", this);
    label_pwd_tip_1->setObjectName("label_pwd_tip_1");
    label_pwd_tip_1->setStyleSheet("color: #ff0000; font-size: 12px;");  // 红色提示
    label_pwd_tip_1->setVisible(false);

    // 设置提示标签位置（groupBox下方）
    int x = ui->groupBox->x();
    int y = ui->groupBox->y() + ui->groupBox->height() + 2;
    label_pwd_tip_0->setGeometry(x, y, 150, 20);
    label_pwd_tip_1->setGeometry(x, y, 150, 20);
}

// 初始化注册按钮（设置样式和初始状态）
void EnrollWindow::init_pushbutton_enroll()
{
    ui->pushButton_enroll->setStyleSheet(
        "QPushButton {"
        "   background-color: #009cff;"
        "   color: rgba(255, 255, 255, 200);"
        "   border-radius: 10px;"
        "   padding: 6px 12px;"
        "   font-family: 'Microsoft YaHei';"
        "   font-size: 14px;"
        "   qproperty-alignment: AlignCenter;"
        "   border: none;"
        "   outline: none;"
        "}"
        "QPushButton:hover:enabled {"
        "   background-color: #007acc;"
        "   color: white;"
        "}"
        "QPushButton:pressed:enabled {"
        "   background-color: #005ca3;"
        "   color: white;"
        "   border: none;"
        "   outline: none;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #98d5fe !important;"  // 强制应用背景色
        "   color: rgba(255, 255, 255, 100) !important;"  // 强制应用文字色
        "   border: none;"
        "   outline: none;"
        "}"
        );

    // 增加样式强制刷新
    ui->pushButton_enroll->style()->unpolish(ui->pushButton_enroll);
    ui->pushButton_enroll->style()->polish(ui->pushButton_enroll);
    ui->pushButton_enroll->update();

    ui->pushButton_enroll->setEnabled(false);
    ui->pushButton_enroll->installEventFilter(this);
}

// 事件过滤器：处理控件的焦点、鼠标悬停等事件
bool EnrollWindow::eventFilter(QObject *watched, QEvent *event)
{
    // 处理注册按钮的鼠标进入/离开事件（切换光标样式）
    if (watched == ui->pushButton_enroll) {
        bool isEnabled = ui->pushButton_enroll->isEnabled();
        if (event->type() == QEvent::Enter) {
            if (!isEnabled) {
                QApplication::setOverrideCursor(Qt::ForbiddenCursor);
            }
            return true;
        } else if (event->type() == QEvent::Leave) {
            // 离开时恢复光标
            QApplication::restoreOverrideCursor();
            return true;
        }
    }

    // 处理姓名输入框的焦点事件（显示/隐藏验证提示）
    if (watched == ui->lineEdit_name) {
        QLabel *tipLabel = findChild<QLabel*>("label_name_tip");
        if (event->type() == QEvent::FocusOut) {  // 失去焦点时验证
            QString text = ui->lineEdit_name->text().trimmed();
            if (text.isEmpty()) {  // 为空时显示错误状态
                ui->lineEdit_name->setStyleSheet(
                    "QLineEdit {"
                    "   qproperty-alignment: 'AlignLeft';"
                    "   padding-left: 10px;"
                    "   background-color: white;"
                    "   font-family: 'Microsoft YaHei';"
                    "   font-size: 14px;"
                    "   border-radius: 10px;"
                    "   border: 1px solid #ff0000;"  // 红色边框
                    "}"
                    "QLineEdit:focus {"
                    "   border: 1px solid #0099ff;"
                    "}"
                    );
                if (tipLabel) tipLabel->setVisible(true);
            } else {  // 输入有效时恢复正常状态
                ui->lineEdit_name->setStyleSheet(
                    "QLineEdit {"
                    "   qproperty-alignment: 'AlignLeft';"
                    "   padding-left: 10px;"
                    "   background-color: white;"
                    "   font-family: 'Microsoft YaHei';"
                    "   font-size: 14px;"
                    "   border-radius: 10px;"
                    "   border: 1px solid #CCCCCC;"
                    "}"
                    "QLineEdit:focus {"
                    "   border: 1px solid #0099ff;"
                    "}"
                    );
                if (tipLabel) tipLabel->setVisible(false);
            }
        } else if (event->type() == QEvent::FocusIn) {  // 获得焦点时隐藏提示
            if (tipLabel) tipLabel->setVisible(false);
        }
    }
    // 处理手机号输入框的焦点事件（显示/隐藏验证提示）
    else if (watched == ui->lineEdit_phonenumber) {
        QLabel *phoneTip = findChild<QLabel*>("label_phonenumber_tip");
        if (event->type() == QEvent::FocusOut) {  // 失去焦点时验证
            QString phoneNumber = ui->lineEdit_phonenumber->text().trimmed();
            if (phoneNumber.isEmpty()) {  // 为空时显示错误
                ui->lineEdit_phonenumber->setStyleSheet(
                    "QLineEdit {"
                    "   qproperty-alignment: 'AlignLeft';"
                    "   padding-left: 10px;"
                    "   background-color: white;"
                    "   font-family: 'Microsoft YaHei';"
                    "   font-size: 14px;"
                    "   border-radius: 10px;"
                    "   border: 1px solid #ff0000;"  // 红色边框
                    "}"
                    "QLineEdit:focus {"
                    "   border: 1px solid #0099ff;"
                    "}"
                    );
                if (phoneTip) {
                    phoneTip->setText("手机号码不能为空");
                    phoneTip->setVisible(true);
                }
            } else if (phoneNumber.length() != 11) {  // 长度不是11位时显示错误
                ui->lineEdit_phonenumber->setStyleSheet(
                    "QLineEdit {"
                    "   qproperty-alignment: 'AlignLeft';"
                    "   padding-left: 10px;"
                    "   background-color: white;"
                    "   font-family: 'Microsoft YaHei';"
                    "   font-size: 14px;"
                    "   border-radius: 10px;"
                    "   border: 1px solid #ff0000;"  // 红色边框
                    "}"
                    "QLineEdit:focus {"
                    "   border: 1px solid #0099ff;"
                    "}"
                    );
                if (phoneTip) {
                    phoneTip->setText("手机号码为11位");
                    phoneTip->setVisible(true);
                }
            } else {  // 输入有效时恢复正常状态
                ui->lineEdit_phonenumber->setStyleSheet(
                    "QLineEdit {"
                    "   qproperty-alignment: 'AlignLeft';"
                    "   padding-left: 10px;"
                    "   background-color: white;"
                    "   font-family: 'Microsoft YaHei';"
                    "   font-size: 14px;"
                    "   border-radius: 10px;"
                    "   border: 1px solid #CCCCCC;"
                    "}"
                    "QLineEdit:focus {"
                    "   border: 1px solid #0099ff;"
                    "}"
                    );
                if (phoneTip) phoneTip->setVisible(false);
            }
        } else if (event->type() == QEvent::FocusIn) {  // 获得焦点时隐藏提示
            if (phoneTip) phoneTip->setVisible(false);
        }
    }
    // 处理密码输入框的焦点事件（显示/隐藏验证提示）
    else if (watched == ui->lineEdit_enroll_password) {
        QLabel *pwdTip0 = findChild<QLabel*>("label_pwd_tip_0");
        QLabel *pwdTip1 = findChild<QLabel*>("label_pwd_tip_1");

        if (event->type() == QEvent::FocusOut) {  // 失去焦点时验证
            QString password = ui->lineEdit_enroll_password->text().trimmed();
            int length = password.length();

            // 先隐藏所有提示
            if (pwdTip0) pwdTip0->setVisible(false);
            if (pwdTip1) pwdTip1->setVisible(false);

            if (length == 0) {  // 密码为空
                ui->groupBox->setStyleSheet("QGroupBox {"
                                            "background-color: white;"
                                            "border: 1px solid #ff0000;"  // 红色边框
                                            "border-radius: 10px;"
                                            "margin-top: 10px;"
                                            "}"
                                            "}");
                if (pwdTip0) pwdTip0->setVisible(true);
            } else if (length < 8 || length > 16) {  // 密码长度不符合要求
                ui->groupBox->setStyleSheet("QGroupBox {"
                                            "background-color: white;"
                                            "border: 1px solid #ff0000;"  // 红色边框
                                            "border-radius: 10px;"
                                            "margin-top: 10px;"
                                            "}"
                                            "}");
                if (pwdTip1) pwdTip1->setVisible(true);
            }
        } else if (event->type() == QEvent::FocusIn) {  // 获得焦点时隐藏提示
            // 隐藏所有提示
            if (pwdTip0) pwdTip0->setVisible(false);
            if (pwdTip1) pwdTip1->setVisible(false);
            // 恢复默认边框（不显示错误状态）
            ui->groupBox->setStyleSheet("QGroupBox {"
                                        "background-color: white;"
                                        "border: 1px solid #CCCCCC;"
                                        "border-radius: 10px;"
                                        "margin-top: 10px;"
                                        "}"
                                        "}");
        }
    }

    return QWidget::eventFilter(watched, event);  // 其他事件交给父类处理
}

// 检查所有输入是否有效，控制注册按钮的可用性
void EnrollWindow::checkAllValid()
{
    QString name = ui->lineEdit_name->text().trimmed();
    bool isNameValid = !name.isEmpty();  // 姓名非空即有效

    QString password = ui->lineEdit_enroll_password->text().trimmed();
    bool isPwdValid = (password.length() >= 8 && password.length() <= 16);  // 密码长度8-16位

    QString phone = ui->lineEdit_phonenumber->text().trimmed();
    bool isPhoneValid = (!phone.isEmpty() && phone.length() == 11);  // 手机号非空且11位

    // 所有输入都有效时启用注册按钮
    bool allValid = isNameValid && isPwdValid && isPhoneValid;
    ui->pushButton_enroll->setEnabled(allValid);

    // 若鼠标在按钮上，实时更新光标样式
    if (ui->pushButton_enroll->underMouse()) {
        QApplication::setOverrideCursor(allValid ? Qt::ArrowCursor : Qt::ForbiddenCursor);
    }
}

// 重绘事件：绘制渐变背景
void EnrollWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);  // 忽略未使用的参数
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);  // 启用抗锯齿

    // 创建渐变背景（从左上角到右下角）
    QLinearGradient gradient(0, 0, width(), height());
    gradient.setColorAt(0, m_color1);  // 起始色
    gradient.setColorAt(1, m_color2);  // 结束色

    // 绘制带圆角的背景
    QRectF windowRect = rect().adjusted(1, 1, -1, -1);  // 向内缩进1px
    int radius = 10;  // 圆角半径
    painter.setBrush(gradient);  // 使用渐变作为画刷
    painter.setPen(Qt::NoPen);   // 无边框
    painter.drawRoundedRect(windowRect, radius, radius);  // 绘制圆角矩形
}

// 鼠标按下事件：实现窗口拖动功能
void EnrollWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {  // 左键按下
        QWidget *child = childAt(event->pos());  // 检查点击位置是否有子控件
        if (!child) {  // 未点击子控件时允许拖动
            mouse_press = true;  // 标记鼠标已按下
            // 记录鼠标相对于窗口的位置
            mousePoint = event->globalPosition().toPoint() - this->pos();
            // 清除输入框焦点
            ui->lineEdit_name->clearFocus();
            ui->lineEdit_enroll_password->clearFocus();
            ui->lineEdit_phonenumber->clearFocus();
        }
    }
}

// 鼠标移动事件：处理窗口拖动
void EnrollWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (mouse_press) {  // 鼠标已按下时移动窗口
        move(event->globalPosition().toPoint() - mousePoint);  // 根据鼠标位置计算窗口新位置
    }
}

// 鼠标释放事件：结束窗口拖动
void EnrollWindow::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    mouse_press = false;  // 标记鼠标已释放
}

// 键盘事件：处理回车键触发注册
void EnrollWindow::keyPressEvent(QKeyEvent *event)
{
    // 回车键且注册按钮可用时触发注册
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        && ui->pushButton_enroll->isEnabled()) {
        on_pushButton_enroll_clicked();
        event->accept();  // 接受事件，不再传递
    } else {
        QWidget::keyPressEvent(event);  // 其他按键交给父类处理
    }
}

// 注册按钮点击事件：启动注册流程
void EnrollWindow::on_pushButton_enroll_clicked()
{
    // 点击后立即禁用按钮，防止重复点击
    ui->pushButton_enroll->setEnabled(false);
    // 显示等待光标，提示用户操作正在进行
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // 获取输入的注册信息
    QString phoneNumber = ui->lineEdit_phonenumber->text().trimmed();
    QString name = ui->lineEdit_name->text().trimmed();
    QString password = ui->lineEdit_enroll_password->text().trimmed();
    // 记录注册开始日志
    writeEnrollWindowLog(QString("开始注册 - 手机号: %1, 昵称: %2").arg(phoneNumber).arg(name));

    // 创建子线程和工作对象（避免UI阻塞）
    QThread* workerThread = new QThread(this);
    EnrollWorker* worker = new EnrollWorker(phoneNumber, name, password);
    worker->moveToThread(workerThread);  // 将工作对象移到子线程

    // 连接信号槽：处理注册结果和线程管理
    connect(worker, &EnrollWorker::enrollResult, this, &EnrollWindow::onEnrollResult);
    connect(worker, &EnrollWorker::errorOccurred, this, &EnrollWindow::onErrorOccurred);
    connect(workerThread, &QThread::started, worker, &EnrollWorker::doEnroll);  // 线程启动后执行注册
    connect(worker, &EnrollWorker::finished, workerThread, &QThread::quit);  // 注册完成后退出线程
    connect(worker, &EnrollWorker::finished, worker, &EnrollWorker::deleteLater);  // 释放工作对象
    connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);  // 释放线程

    workerThread->start();  // 启动线程
}

// 密码显示按钮按下事件：显示密码明文
void EnrollWindow::on_pushButton_show_password_pressed()
{
    QPixmap pix(":/images/show_password.png");  // 切换为显示密码图标
    ui->pushButton_show_password->setIcon(pix);
    ui->pushButton_show_password->setIconSize(QSize(20, 20));
    ui->lineEdit_enroll_password->setEchoMode(QLineEdit::Normal);  // 切换为明文模式
}

// 密码显示按钮释放事件：隐藏密码
void EnrollWindow::on_pushButton_show_password_released()
{
    QPixmap pix(":/images/hide_password.png");  // 切换为隐藏密码图标
    ui->pushButton_show_password->setIcon(pix);
    ui->pushButton_show_password->setIconSize(QSize(20, 20));
    ui->lineEdit_enroll_password->setEchoMode(QLineEdit::Password);  // 切换为密码模式
}

// 处理注册结果：显示成功/失败信息
void EnrollWindow::onEnrollResult(bool success, const QString& message)
{
    // 恢复光标和按钮状态
    QApplication::restoreOverrideCursor();
    ui->pushButton_enroll->setEnabled(true);
    // 记录注册结果日志
    QString phoneNumber = ui->lineEdit_phonenumber->text().trimmed(); // 需要先获取手机号
    writeEnrollWindowLog(
        QString("注册%1 - 手机号: %2, 信息: %3")
            .arg(success ? "成功" : "失败")
            .arg(phoneNumber)
            .arg(message),
        success ? "INFO" : "ERROR"
        );

    // 创建注册状态窗口
    EnrollStateWidget* resultWidget = new EnrollStateWidget();
    // 连接"重新注册"信号：清除输入并重置状态
    connect(resultWidget, &EnrollStateWidget::reenrollRequested, this, [=](){
        // 清除输入框内容
        ui->lineEdit_phonenumber->clear();

        // 清除输入框焦点
        ui->lineEdit_name->clearFocus();
        ui->lineEdit_enroll_password->clearFocus();
        ui->lineEdit_phonenumber->clearFocus();

        // 重置密码框样式和提示标签
        ui->groupBox->setStyleSheet("QGroupBox {"
                                    "background-color: white;"
                                    "border: 1px solid #CCCCCC;"
                                    "border-radius: 10px;"
                                    "margin-top: 10px;"
                                    "}"
                                    "}");
        QLabel *pwdTip0 = findChild<QLabel*>("label_pwd_tip_0");
        QLabel *pwdTip1 = findChild<QLabel*>("label_pwd_tip_1");
        if (pwdTip0) pwdTip0->setVisible(false);
        if (pwdTip1) pwdTip1->setVisible(false);

        // 重置姓名输入框样式和提示
        ui->lineEdit_name->setStyleSheet(
            "QLineEdit {"
            "   qproperty-alignment: 'AlignLeft';"
            "   padding-left: 10px;"
            "   background-color: white;"
            "   font-family: 'Microsoft YaHei';"
            "   font-size: 14px;"
            "   border-radius: 10px;"
            "   border: 1px solid #CCCCCC;"
            "}"
            "QLineEdit:focus {"
            "   border: 1px solid #0099ff;"
            "}"
            );
        QLabel *nameTip = findChild<QLabel*>("label_name_tip");
        if (nameTip) nameTip->setVisible(false);

        // 重置手机号输入框样式和提示
        ui->lineEdit_phonenumber->setStyleSheet(
            "QLineEdit {"
            "   qproperty-alignment: 'AlignLeft';"
            "   padding-left: 10px;"
            "   background-color: white;"
            "   font-family: 'Microsoft YaHei';"
            "   font-size: 14px;"
            "   border-radius: 10px;"
            "   border: 1px solid #CCCCCC;"
            "}"
            "QLineEdit:focus {"
            "   border: 1px solid #0099ff;"
            "}"
            );
        QLabel *phoneTip = findChild<QLabel*>("label_phonenumber_tip");
        if (phoneTip) phoneTip->setVisible(false);

        // 让窗口自身获取焦点
        this->setFocus(Qt::OtherFocusReason);

        checkAllValid();  // 重新检查输入状态
        this->raise();  // 窗口置顶
        this->activateWindow();  // 激活窗口
        ui->pushButton_enroll->setEnabled(true); // 显式启用按钮
        checkAllValid();
    });

    // 连接"返回登录"信号：切换到登录窗口
    connect(resultWidget, &EnrollStateWidget::backToloadRequested, this, [=](){
        emit backToLoginRequested();  // 发射返回登录信号
        this->close();  // 关闭注册窗口
    });
    connect(this, &EnrollWindow::close, this, [=](){
        emit backToLoginRequested();  // 发射返回登录信号

    });

    // 获取状态窗口中的控件
    QPushButton* reenrollBtn = resultWidget->findChild<QPushButton*>("reenrollBtn");
    QPushButton* backToLoginBtn = resultWidget->findChild<QPushButton*>("backToloadBtn");
    QLabel* titleLabel = resultWidget->findChild<QLabel*>("titleLabel");
    QLabel* msgLabel = resultWidget->findChild<QLabel*>("msgLabel");

    if (success) {  // 注册成功
        if (titleLabel) titleLabel->setText("注册成功");
        if (msgLabel) msgLabel->setText("欢迎使用QQ");

        // 隐藏重新注册按钮，显示返回登录按钮
        if (reenrollBtn) reenrollBtn->setVisible(false);
        if (backToLoginBtn) backToLoginBtn->setVisible(true);
    } else {  // 注册失败
        if (msgLabel) msgLabel->setText(message);  // 显示失败原因

        // 显示重新注册按钮，隐藏返回登录按钮
        if (reenrollBtn) reenrollBtn->setVisible(true);
        if (backToLoginBtn) backToLoginBtn->setVisible(false);
    }

    // 使状态窗口居中显示在注册窗口中央
    QPoint enrollCenter = this->geometry().center();
    QRect errorRect = resultWidget->geometry();
    resultWidget->move(
        enrollCenter.x() - errorRect.width()/2,
        enrollCenter.y() - errorRect.height()/2
        );

    // 设置状态窗口为置顶对话框
    resultWidget->setWindowFlags(resultWidget->windowFlags() | Qt::WindowStaysOnTopHint | Qt::Dialog);
    resultWidget->show();  // 显示窗口
    resultWidget->raise();
    resultWidget->activateWindow();
    resultWidget->setAttribute(Qt::WA_DeleteOnClose);  // 关闭时自动释放


}

// 处理注册过程中的错误（网络错误等）
void EnrollWindow::onErrorOccurred(const QString& errorMsg)
{

    // 恢复光标和按钮状态
    QApplication::restoreOverrideCursor();
    ui->pushButton_enroll->setEnabled(true);
    // 记录错误日志
    writeEnrollWindowLog(errorMsg, "ERROR");
    // 显示错误信息
    QMessageBox::critical(this, "操作失败", errorMsg, QMessageBox::Ok);
}
