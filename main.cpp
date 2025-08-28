#include "widget.h"
#include "loginstatewidget.h"

#include <QApplication>
#include"load.h"
int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    load w;
    w.show();
    return a.exec();
}
