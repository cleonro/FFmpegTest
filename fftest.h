#ifndef FFTEST_H
#define FFTEST_H

#include <QScopedPointer>
#include <QThread>
#include <QTimer>

#include <QObject>
#include <QQueue>
#include <QMutex>

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

    int hasSBR1(AVCodecContext *codecContext);
    int hasSBR2(AVCodecContext *codecContext);
    int hasSBR3(AVCodecContext *codecContext);
}

class QIODevice;
class QAudioOutput;

#define TEST_SBR 0

/*
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
//    void initTimer();

public slots:
//    void onTimer();
//    void onInitTimer();

    void onDecodedFrame(QByteArray *buffer, double ptime);

private:
    void writeToAudio(QByteArray *buffer);

private:
    FFTest *m_ffTest;

    QThread m_thread;
    QMutex m_mutex;
    QTimer m_timer;

    double m_lastpTime;
    QQueue<AudioFrame> m_playQueue;
};
*/

class FFTest : public QObject
{
    friend class FFTestOps;

    Q_OBJECT

public:
    enum class State
    {
        IDLE,
        OPENING,
        DECODE
    };

public:
    explicit FFTest(QObject *parent = nullptr);
    ~FFTest();

    void startThread();

    void init();
    void openRequest(QString filePath);

    void stop();

    void setEncode(bool encode);
    void setSendToAudio(bool sendToAudio);

signals:
    void decodedFrame(QByteArray *buffer, double ptime);
    void openSignal(QString filePath);
    void state(FFTest::State);

private slots:
    void open(QString filePath);

private:
    bool needToStop();

    void clear();
    int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame);

    int doSomething(AVCodecContext *pCodecContext, AVFrame *pFrame);

    std::string avErr2str(int errnum);

    void initAudio(int freq, int channels);
    void writeToAudio__(const char *buffer, qint64 length);
    void writeToAudio(const char *buffer, qint64 length);

    int initEncoder();
    int initResampler();
    int initFifo();

    void checkCloseFifo();

    void findMetadata(AVPacket* pPacket);

private:
    QThread m_thread;
    QMutex m_mutex;
    bool m_stop;
    bool m_closing;

    const char *m_space;

    //FFTestOps m_ffTestOps;
    bool m_sendToAudio;
    bool m_encode;

    //audio
    QScopedPointer<QAudioOutput> m_audioOutput;
    QIODevice *m_audioDevice;

    int m_audio_stream_index = -1;
    int m_video_stream_index = -1;

    //ffmpeg specifics;
    AVFormatContext *pFormatContext;
    const AVCodec *pCodec;
    AVCodecParameters *pCodecParameters;
    AVCodecContext *pCodecContext;

    //encode information
    const char *m_outputFileName;
    AVFormatContext *pEncoderFormatContext;
    AVStream *pEncoderStream;
    const AVCodec *pEncoderCodec;
    AVCodecContext *pEncoderCodecContext;

    SwrContext *pSwrContext;
    AVAudioFifo *pAudioFifo;
    uint8_t **pConvertedSamples;
    bool m_convertedSamplesInitialized;
    int64_t m_encodePTS;

};

#endif // FFTEST_H
