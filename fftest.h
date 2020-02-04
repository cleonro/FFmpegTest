#ifndef FFTEST_H
#define FFTEST_H

#include <QObject>

extern "C"
{
    struct AVFormatContext;
    struct AVCodec;
    struct AVCodecParameters;
    struct AVCodecContext;
    struct AVPacket;
    struct AVFrame;
}

class FFTest : public QObject
{
    Q_OBJECT
public:
    explicit FFTest(QObject *parent = nullptr);
    ~FFTest();

    void init();
    void open(const char *filePath);

signals:

private:
    void clear();
    int decode_packet(AVPacket *, AVCodecContext *pCodecContext, AVFrame *pFrame);

private:
    const char *m_space;

    //ffmpeg specifics;
    AVFormatContext *pFormatContext;
    AVCodec *pCodec;
    AVCodecParameters *pCodecParameters;
    AVCodecContext *pCodecContext;

};

#endif // FFTEST_H
