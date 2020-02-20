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
/// 6. Google - search "libavcodec get audio sample value" [third result]
///     https://steemit.com/programming/@targodan/decoding-audio-files-with-ffmpeg
///     https://gist.github.com/targodan/8cef8f2b682a30055aa7937060cd94b7
///
/// 7. Google - search "qaudiooutput period size"
///     https://stackoverflow.com/questions/32049950/realtime-streaming-with-qaudiooutput-qt
///
/// 8. live streaming with libav [or ffmpeg]
///
///     https://blog.mi.hdm-stuttgart.de/index.php/2018/03/21/livestreaming-with-libav-tutorial-part-2/
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
