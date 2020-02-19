#ifndef FFTEST_H
#define FFTEST_H

#include <QScopedPointer>
#include <QThread>
#include <QTimer>

#include <QObject>
#include <QQueue>

extern "C"
{
    struct AVFormatContext;
    struct AVCodec;
    struct AVCodecParameters;
    struct AVCodecContext;
    struct AVPacket;
    struct AVFrame;
    struct AVStream;

    struct SwrContext;

    struct AVAudioFifo;
}

class QIODevice;
class QAudioOutput;

class FFTest;

class FFTestOps : public QObject
{
    Q_OBJECT
    struct AudioFrame
    {
        QByteArray *buffer = nullptr;
        double pTime = 0;
    };

public:
    FFTestOps(FFTest *ffTest, QObject *parent = nullptr);
    ~FFTestOps();

signals:
    void initTimer();

public slots:
    void onTimer();
    void onInitTimer();

    void onDecodedFrame(QByteArray *buffer, double ptime);

private:
    void writeToAudio(QByteArray *buffer);

private:
    FFTest *m_ffTest;

    QThread m_thread;
    QTimer m_timer;

    double m_lastpTime;
    QQueue<AudioFrame> m_playQueue;
};

class FFTest : public QObject
{
    friend class FFTestOps;

    Q_OBJECT
public:
    explicit FFTest(QObject *parent = nullptr);
    ~FFTest();

    void init();
    void openRequest(QString filePath);

signals:
    void decodedFrame(QByteArray *buffer, double ptime);
    void openSignal(QString filePath);

private slots:
    void open(QString filePath);

private:
    void clear();
    int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame);

    int doSomething(AVCodecContext *pCodecContext, AVFrame *pFrame);

    std::string avErr2str(int errnum);

    void initAudio(int freq, int channels);
    void writeToAudio(const char *buffer, qint64 length);

    int initEncoder();
    int initResampler();
    int initFifo();

private:
    QThread m_thread;

    const char *m_space;

    FFTestOps m_ffTestOps;

    //audio
    QScopedPointer<QAudioOutput> m_audioOutput;
    QIODevice *m_audioDevice;

    int m_audio_stream_index = -1;
    int m_video_stream_index = -1;

    //ffmpeg specifics;
    AVFormatContext *pFormatContext;
    AVCodec *pCodec;
    AVCodecParameters *pCodecParameters;
    AVCodecContext *pCodecContext;

    //encode information
    const char *m_outputFileName;
    AVFormatContext *pEncoderFormatContext;
    AVStream *pEncoderStream;
    AVCodec *pEncoderCodec;
    AVCodecContext *pEncoderCodecContext;

    SwrContext *pSwrContext;
    AVAudioFifo *pAudioFifo;
    uint8_t **pConvertedSamples;
    bool m_convertedSamplesInitialized;
    int64_t m_encodePTS;

};

#endif // FFTEST_H
