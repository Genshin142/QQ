#ifndef ENROLLSTATEWIDGET_H
#define ENROLLSTATEWIDGET_H

#include <QWidget>

namespace Ui {
class EnrollStateWidget;
}

// 注册状态窗口类,用于展示注册过程中的状态信息(加载/成功/失败)
class EnrollStateWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EnrollStateWidget(QWidget *parent = nullptr);  // 构造函数,parent为父窗口指针
    ~EnrollStateWidget();                                   // 析构函数,释放UI资源
    void paintEvent(QPaintEvent *event) override;           // 重写绘制事件,处理自定义界面绘制
    void moveEvent(QMoveEvent *event) override;             // 重写移动事件,处理窗口位置变化逻辑
    void initWidgets();                                     // 初始化界面控件(样式/布局/状态)

signals:
    void reenrollRequested();       // 重新注册信号,用户点击"重新注册"时触发
    void backToloadRequested();     // 返回登录信号,用户点击"返回登录"时触发


private:
    Ui::EnrollStateWidget *ui;
};

#endif // ENROLLSTATEWIDGET_H
