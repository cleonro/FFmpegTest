#include "ffmainwindow.h"

#include <QApplication>

int main(int argv, char *argc[])
{
    qInstallMessageHandler(messageOutput);

    QApplication a(argv, argc);
    FFMainWindow w;
    w.show();

    int r = a.exec();

    return r;
}
