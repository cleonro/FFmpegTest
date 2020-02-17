#include "ffmainwindow.h"

///TUTORIALS links
/// 1.
/// https://github.com/leandromoreira/ffmpeg-libav-tutorial
///
///     1.1
///     http://leixiaohua1020.github.io/#ffmpeg-development-examples
///
///     1.2
///     https://slhck.info/ffmpeg-encoding-course/#/9
///
/// 2. (older)
/// http://dranger.com/ffmpeg/
///
/// 3. (not investigated - yet)
/// https://www.whoishostingthis.com/compare/ffmpeg/resources/
///
/// 4. Google - search "construct avframe from buffer"
///     https://ffmpeg.zeranoe.com/forum/viewtopic.php?t=6036
///
/// 5. Google - search "AVFrame extended_data"
///     https://www.ffmpeg.org/doxygen/2.3/decoding_encoding_8c-example.html#_a15
///


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
