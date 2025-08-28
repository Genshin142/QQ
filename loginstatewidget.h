#ifndef LOGINSTATEWIDGET_H
#define LOGINSTATEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QPainter>

namespace Ui {
class LoginStateWidget;
}

class LoginStateWidget : public QWidget  // 登录状态窗口类(展示登录过程状态或结果)
{
    Q_OBJECT

public:
    explicit LoginStateWidget(QWidget *parent = nullptr);  // 构造函数,parent为父窗口指针
    ~LoginStateWidget();                                   // 析构函数,释放UI资源
    void paintEvent(QPaintEvent *event) override;          // 重写绘制事件(自定义窗口外观)
    void moveEvent(QMoveEvent *event) override;            // 重写移动事件(处理窗口位置逻辑)
    void initWidgets();                                    // 初始化界面控件(样式/布局/信号连接)

signals:
    void reloginRequested();                               // 重新登录信号(用户触发重新登录时发射)


private:
    Ui::LoginStateWidget *ui;
};

#endif // LOGINSTATEWIDGET_H
