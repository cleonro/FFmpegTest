#ifndef FFTEST_H
#define FFTEST_H

#include <QScopedPointer>
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

class QIODevice;
class QAudioOutput;

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
    int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame);

    int doSomething(AVCodecContext *pCodecContext, AVFrame *pFrame);

    std::string avErr2str(int errnum);

    void initAudio();
    void writeToAudio(const char *buffer, int length);

private:
    const char *m_space;

    //audio
    QScopedPointer<QAudioOutput> m_audioOutput;
    QIODevice *m_audioDevice;

    //ffmpeg specifics;
    AVFormatContext *pFormatContext;
    AVCodec *pCodec;
    AVCodecParameters *pCodecParameters;
    AVCodecContext *pCodecContext;

};

#endif // FFTEST_H
