#include "ffmainwindow.h"

///TUTORIALS links
/// 1.
/// https://github.com/leandromoreira/ffmpeg-libav-tutorial
///
/// 2. (older)
/// http://dranger.com/ffmpeg/
///
/// 3. (not investigated - yet)
/// https://www.whoishostingthis.com/compare/ffmpeg/resources/


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
