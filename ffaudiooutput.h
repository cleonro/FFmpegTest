#ifndef FFAUDIOOUTPUT_H
#define FFAUDIOOUTPUT_H

#include <QThread>
#include <QMutex>
#include <QScopedPointer>

class QAudioOutput;
class QIODevice;

class FFAudioOutput : public QObject
{
    Q_OBJECT
public:
    explicit FFAudioOutput(QObject *parent = nullptr);
    ~FFAudioOutput();

    void init(int sample_rate, int channels, int frame_size);

signals:

private slots:
    void onAudioNotify();

private:
    const char *m_space;

    QThread m_thread;
    QMutex m_mutex;

    QScopedPointer<QAudioOutput> m_audioOutput;
    QIODevice *m_audioOutputBuffer;

};

#endif // FFAUDIOOUTPUT_H
