#ifndef ENROLLWINDOW_H
#define ENROLLWINDOW_H

#include <QWidget>
#include <QColor>
#include <QPropertyAnimation>
#include <QPoint>

namespace Ui {
class EnrollWindow;
}

class EnrollWindow : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor color1 READ color1 WRITE setColor1)  // 背景渐变颜色属性1(用于动画)
    Q_PROPERTY(QColor color2 READ color2 WRITE setColor2)  // 背景渐变颜色属性2(用于动画)

public:
    explicit EnrollWindow(QWidget *parent = nullptr);      // 构造函数,parent为父窗口指针
    ~EnrollWindow();                                       // 析构函数,释放UI资源

    QColor color1() const { return m_color1; }             // 获取背景颜色1
    void setColor1(const QColor &color) { m_color1 = color; update(); }  // 设置背景颜色1并触发重绘
    QColor color2() const { return m_color2; }             // 获取背景颜色2
    void setColor2(const QColor &color) { m_color2 = color; update(); }  // 设置背景颜色2并触发重绘

    void init_lineEdit_name();                             // 初始化姓名输入框(样式/验证)
    void init_lineEdit_enroll_password();                  // 初始化密码输入框(样式/密码模式)
    void init_label_welcome();                             // 初始化欢迎标签(样式/文本)
    void init_lineEdit_phonenumber();                      // 初始化手机号输入框(样式/验证)
    void init_pushbutton_enroll();                         // 初始化注册按钮(样式/状态)
    void init_pushbutton_show();                           // 初始化密码显示/隐藏按钮(样式/交互)
    void init_groupbox();                                  // 初始化密码输入框容器(样式)
    void init_background_color();                          // 初始化背景渐变动画(颜色/时长)
    void keyPressEvent(QKeyEvent *event) override;         // 重写键盘事件(处理回车键注册)
    void checkAllValid();                                  // 检查输入合法性(控制注册按钮状态)
    bool eventFilter(QObject *watched, QEvent *event) override;  // 事件过滤器(处理子控件交互)
    void save_account();                                   // 保存注册账号信息(可选功能)

signals:
    void startEnroll(const QString& phoneNumber, const QString& name, const QString& password);  // 发送注册请求信号
    void backToLoginRequested();                           // 返回登录窗口信号

private slots:
    void on_pushButton_enroll_clicked();                   // 注册按钮点击事件
    void on_pushButton_show_password_pressed();            // 密码显示按钮按下事件(明文显示)
    void on_pushButton_show_password_released();           // 密码显示按钮释放事件(隐藏密码)
    void onEnrollResult(bool success, const QString& message);  // 处理注册结果(成功/失败)
    void onErrorOccurred(const QString& errorMsg);         // 处理注册错误(网络/解析错误等)

protected:
    void paintEvent(QPaintEvent *event) override;          // 重绘事件(绘制背景渐变)
    void mousePressEvent(QMouseEvent *event) override;     // 鼠标按下事件(窗口拖动起点)
    void mouseMoveEvent(QMouseEvent *event) override;      // 鼠标移动事件(实现窗口拖动)
    void mouseReleaseEvent(QMouseEvent *event) override;   // 鼠标释放事件(结束窗口拖动)

private:
    Ui::EnrollWindow *ui;                                  // UI类实例指针(管理界面控件)
    QColor m_color1;                                       // 背景渐变起始色
    QColor m_color2;                                       // 背景渐变结束色
    QPropertyAnimation *m_anim1;                           // 控制color1的动画对象
    QPropertyAnimation *m_anim2;                           // 控制color2的动画对象
    QPoint mousePoint;                                     // 鼠标拖动时的位置记录
    bool mouse_press;                                      // 鼠标按下状态标志(用于拖动)
};

#endif // ENROLLWINDOW_H
