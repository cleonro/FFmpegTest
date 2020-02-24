#include "ffaudiooutput.h"

#include <QMutexLocker>
#include <QAudioOutput>
#include <QDebug>

FFAudioOutput::FFAudioOutput(QObject *parent)
    : QObject(parent)
    , m_space("    ")
    , m_audioOutput(new QAudioOutput())
    , m_audioOutputBuffer(nullptr)
{
    this->moveToThread(&m_thread);

    connect(m_audioOutput.get(), &QAudioOutput::notify, this, &FFAudioOutput::onAudioNotify);
}

FFAudioOutput::~FFAudioOutput()
{
    m_thread.quit();
    m_thread.wait();
}

void FFAudioOutput::init(int sample_rate, int channels, int frame_size)
{
    int sampleSize = 32;//16; //bits per sample

    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultOutputDevice();

    QAudioFormat format;
    format.setSampleRate(sample_rate);
    format.setChannelCount(channels);
    format.setSampleSize(sampleSize);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::Float);
    //format.setSampleType(QAudioFormat::UnSignedInt);

    if(!deviceInfo.isFormatSupported(format))
    {
        qDebug() << "Audio format not supported!";
        format = deviceInfo.nearestFormat(format);
    }

    m_audioOutput.reset(new QAudioOutput(deviceInfo, format));
    m_audioOutput->setBufferSize(32 * 2048);//8192);
    m_audioOutputBuffer = m_audioOutput->start();
    int audioBufferSize = m_audioOutput->bufferSize();
    int audioPeriodSize = m_audioOutput->periodSize();
    qDebug() << m_space << Q_FUNC_INFO << " audio buffer size: " << audioBufferSize
             << "; period size: " << audioPeriodSize;
}

void FFAudioOutput::onAudioNotify()
{

}
