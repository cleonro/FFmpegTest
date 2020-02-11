#include "fftest.h"

#include <QThread>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QByteArray>
#include <QDebug>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    //#include <libswscale/swscale.h>

    #include <libswresample/swresample.h>
}

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

FFTest::FFTest(QObject *parent)
    : QObject(parent)
    , m_space("    ")
    , m_ffTestOps(this)
    , m_audioDevice(nullptr)
    , pFormatContext(nullptr)
    , pCodec(nullptr)
    , pCodecParameters(nullptr)
    , pCodecContext(nullptr)
    , m_outputFileName("FFTest.mp3")
    , pEncoderFormatContext(nullptr)
    , pEncoderStream(nullptr)
    , pEncoderCodec(nullptr)
    , pEncoderCodecContext(nullptr)
{

}

FFTest::~FFTest()
{
    clear();
}

void FFTest::clear()
{
//    delete pFormatContext;
    //delete pCodec;
    //delete pCodecParameters;
//    pCodec = nullptr;
//    pCodecParameters = nullptr;

    avformat_close_input(&pFormatContext);
    avformat_free_context(pFormatContext);

    avcodec_free_context(&pCodecContext);
}

void FFTest::init()
{
    clear();

    qDebug() << Q_FUNC_INFO;

    qDebug() << m_space << avformat_license();

    //deprecated
    //av_register_all();

    pFormatContext = avformat_alloc_context();
    if(pFormatContext == nullptr)
    {
        qDebug() << m_space << "ERROR could not allocate memory for Format Context";
    }
    else
    {
        qDebug() << m_space << "Format Context allocated!";
    }
}

void FFTest::open(const char *filePath)
{
    int audioFreq = 0;
    int audioChannels = 0;

    clear();

    qDebug() << Q_FUNC_INFO;
    int status = avformat_open_input(&pFormatContext, filePath, nullptr, nullptr);
    if(status != 0)
    {
        qDebug() << m_space << "ERROR could not open the file";
    }
    qDebug() << m_space << "File opened!";

    //fmpeg info
    qDebug() << m_space << m_space << "format " << pFormatContext->iformat->name;
    qDebug() << m_space << m_space << "format long name " << pFormatContext->iformat->long_name;
    qDebug() << m_space << m_space << "duration " << pFormatContext->duration;
    qDebug() << m_space << m_space << "bit_rate" << pFormatContext->bit_rate;

    status = avformat_find_stream_info(pFormatContext, nullptr);
    if(status < 0)
    {
        qDebug() << m_space << m_space << "ERROR could not get the stream info";
    }
    qDebug() << m_space << m_space << "Stream info found! " << status;

    // loop though all the streams and print its main information
    qDebug() << m_space << m_space << "Number of streams "
             << pFormatContext->nb_streams;

    int audio_stream_index = -1;
    int video_stream_index = -1;

    for(int i = 0; i < pFormatContext->nb_streams; ++i)
    {
        qDebug() << m_space << m_space << m_space
                 << "Stream " << i;
        AVCodecParameters *pLocalCodecParameters = nullptr;
        pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
        qDebug() << m_space << m_space << m_space
                 << "AVStream::time_base before open coded "
                 << pFormatContext->streams[i]->time_base.num << "/"
                 << pFormatContext->streams[i]->time_base.den;
        qDebug() << m_space << m_space << m_space
                 << "AVStream::r_frame_rate before open coded "
                 << pFormatContext->streams[i]->r_frame_rate.num << "/"
                 << pFormatContext->streams[i]->r_frame_rate.den;
        qDebug() << m_space << m_space << m_space
                 << "AVStream::start_time "
                 << pFormatContext->streams[i]->start_time;
        qDebug() << m_space << m_space << m_space
                 << "AVStream::duration "
                 << pFormatContext->streams[i]->duration;

        AVCodec *pLocalCodec = nullptr;
        pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
        if(pLocalCodec == nullptr)
        {
            qDebug() << m_space << m_space << m_space
                     << "ERROR unsupported codec!";
            continue;
        }
        qDebug() << m_space << m_space << m_space
                 << "Found supported codec!";

        switch(pLocalCodecParameters->codec_type)
        {
        case AVMEDIA_TYPE_VIDEO:
            qDebug() << m_space << m_space << m_space
                     << "Codec type: AVMEDIA_TYPE_VIDEO";

//            if(video_stream_index == -1)
//            {
//                video_stream_index = i;
//                pCodec = pLocalCodec;
//                pCodecParameters = pLocalCodecParameters;
//            }

            qDebug() << m_space << m_space << m_space
                     << "Video codec resolution: " << pLocalCodecParameters->width
                     << "x"
                     << pLocalCodecParameters->height;

            break;

        case AVMEDIA_TYPE_AUDIO:
            qDebug() << m_space << m_space << m_space
                     << "Codec type: AVMEDIA_TYPE_AUDIO";
            if(audio_stream_index == -1)
            {
                audio_stream_index = i;
                pCodec = pLocalCodec;
                pCodecParameters = pLocalCodecParameters;
            }

            qDebug() << m_space << m_space << m_space
                     << "Audio Codec - channels: " << pLocalCodecParameters->channels;
            qDebug() << m_space << m_space << m_space
                     << "Audio Codec - sample rate: " << pLocalCodecParameters->sample_rate;

            audioFreq = pLocalCodecParameters->sample_rate;
            audioChannels = pLocalCodecParameters->channels;

            break;

        default:
            qDebug() << m_space << m_space << m_space
                     << "Codec type: unknown";
            break;
        }

        qDebug() << m_space << m_space << m_space
                 << "Codec  name: " << pLocalCodec->name;
        qDebug() << m_space << m_space << m_space
                 << "Codec id: " << pLocalCodec->id;
        qDebug() << m_space << m_space << m_space
                 << "Codec bit_rate: " << pLocalCodecParameters->bit_rate;

        qDebug() << m_space;
    }

    pCodecContext = avcodec_alloc_context3(pCodec);
    if(pCodecContext == nullptr)
    {
        qDebug() << m_space << m_space << "failed to allocated memory for AVCodecContext";
        return;
    }

    qDebug() << m_space << m_space << "Allocated memory for AVCodecContext";

    status = avcodec_parameters_to_context(pCodecContext, pCodecParameters);
    if(status < 0)
    {
        qDebug() << m_space << m_space << "failed to copy codec params to codec context";
        return;
    }

    qDebug() << m_space << m_space << "Copied codec params to codec context";

    status = avcodec_open2(pCodecContext, pCodec, nullptr);
    if(status < 0)
    {
        qDebug() << m_space << m_space << "failed to open codec through avcodec_open2";
        return;
    }

    qDebug() << m_space << m_space << "Opened codec through avcodec_open2";

    AVFrame *pFrame = av_frame_alloc();
    if(pFrame == nullptr)
    {
        qDebug() << m_space << m_space << "failed to allocated memory for AVFrame";
        return;
    }

    qDebug() << m_space << m_space << "Allocated memory for AVFrame";

    AVPacket *pPacket = av_packet_alloc();
    if(pPacket == nullptr)
    {
        qDebug() << m_space << m_space << "failed to allocated memory for AVPacket";
        return;
    }

    qDebug() << m_space << m_space << "Allocated memory for AVPacket";

    int response = 0;
    int how_many_packets_to_process = 8; //?

    //from https://github.com/leixiaohua1020/simplest_ffmpeg_audio_player/blob/master/simplest_ffmpeg_audio_decoder/simplest_ffmpeg_audio_decoder.cpp
    //{
//        uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
//        int out_nb_samples = pCodecContext->frame_size;
//        AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
//        int out_sample_rate = 44100;
//        int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
//        int out_buffer_size = av_samples_get_buffer_size(nullptr, out_channels,
//                                                         out_nb_samples,
//                                                         out_sample_fmt,
//                                                         1);


//        uint32_t len = 0;
//        int got_picture = 0;

//        SwrContext *au_convert_ctx = nullptr;

//        uint8_t *out_buffer = (uint8_t*)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);

//        int64_t in_channel_layout = av_get_default_channel_layout(pCodecContext->channels);

//        au_convert_ctx = swr_alloc();
//        au_convert_ctx = swr_alloc_set_opts(au_convert_ctx, out_channel_layout,
//                                            out_sample_fmt, out_sample_rate,
//                                            in_channel_layout,
//                                            pCodecContext->sample_fmt,
//                                            pCodecContext->sample_rate,
//                                            0, nullptr);
    //}

    initAudio(audioFreq, audioChannels);
    initEncoder();

    while(av_read_frame(pFormatContext, pPacket) >= 0)
    {

        int packet_stream_index = pPacket->stream_index;
        if(packet_stream_index == audio_stream_index)
        //if(packet_stream_index == video_stream_index)
        {
            qDebug() << m_space << m_space
                     << "AVPacket::pts " << pPacket->pts;

//            response = decode_packet(pPacket, pCodecContext, pFrame);
//            qDebug() << m_space << m_space << m_space
//                     << "decode_packet response: " << response;

            response = avcodec_send_packet(pCodecContext, pPacket);
            if(response < 0)
            {
                qDebug() << m_space << m_space
                         << "Error while sending a packet to the decoder: "
                         << avErr2str(response).c_str();
                continue;
            }
            response = avcodec_receive_frame(pCodecContext, pFrame);
            if(response < 0)
            {
                if(response == AVERROR(EAGAIN) || response == AVERROR_EOF)
                {
                    if(response == AVERROR(EAGAIN))
                    {
                        qDebug() << m_space << m_space
                                 << "avcodec_receive_frame: AVERROR(EAGAIN)";
                    }
                    if(response == AVERROR_EOF)
                    {
                        qDebug() << m_space << m_space
                                 << "avcodec_receive_frame: AVERROR_EOF";
                    }
                }
                else if(response < 0)
                {
                    qDebug() << m_space << m_space
                             << "Error while receiving a frame from the decoder: "
                             << avErr2str(response).c_str();
                }
                continue;
            }
            doSomething(pCodecContext, pFrame);


            if(--how_many_packets_to_process <= 0)
            {
                //break;
            }

            //from https://github.com/leixiaohua1020/simplest_ffmpeg_audio_player/blob/master/simplest_ffmpeg_audio_decoder/simplest_ffmpeg_audio_decoder.cpp
            //{

            //DEPRECATED
            //response = avcodec_decode_audio4(pCodecContext, pFrame,
            //                                     &got_picture, pPacket);
            //if(response < 0)
            //{
            //    qDebug() << m_space << m_space
            //             << "avcodec_decode_audio4 error: "
            //             << avErr2str(response).c_str();
            //    return;
            //}

            //}

            av_packet_unref(pPacket);
        }
    }

    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
}

int FFTest::decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame)
{
    int response = 0;

    qDebug() << "\n" << Q_FUNC_INFO;

    response = avcodec_send_packet(pCodecContext, pPacket);
    if(response < 0)
    {
        qDebug() << m_space << "Error while sending a packet to the decoder: "
                 << avErr2str(response).c_str();
    }
    qDebug() << m_space << "Packet sent to the decoder";

    while(response >= 0)
    {
        response = avcodec_receive_frame(pCodecContext, pFrame);
        if(response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            if(response == AVERROR(EAGAIN))
            {
                qDebug() << m_space << m_space
                         << "avcodec_receive_frame: AVERROR(EAGAIN)";
            }
            if(response == AVERROR_EOF)
            {
                qDebug() << m_space << m_space
                         << "avcodec_receive_frame: AVERROR_EOF";
            }
            break;
        }
        else if(response < 0)
        {
            qDebug() << m_space << m_space
                     << "Error while receiving a frame from the decoder: "
                     << avErr2str(response).c_str();
            return response;
        }

        if(response >= 0)
        {
//            qDebug() << m_space << m_space
//                     << "Frame " << pCodecContext->frame_number
//                     << " (type = " << av_get_s

            qDebug() << m_space << m_space
                     << "Frame " << pCodecContext->frame_number
                     << ", " << pCodecContext->channels << " channels, "
                     << pFrame->nb_samples << " samples, "
                     << pCodecContext->sample_fmt << " sample_fmt";

        }
    }

    qDebug() << "\n";
    return response;
}

int FFTest::doSomething(AVCodecContext *pCodecContext, AVFrame *pFrame)
{
    qDebug() << m_space << Q_FUNC_INFO;

    int response = 0;

    qDebug() << m_space << m_space
             << "Frame " << pCodecContext->frame_number
             << " [codec context], "
             << pCodecContext->channels << " channels [codec context], "
             << pFrame->nb_samples << " samples [frame], "
             << pCodecContext->sample_fmt << " sample_fmt [codec context], "
             << pFrame->pts << " pts[frame],"
             << pFrame->pkt_dts << " pkt_dts[frame], "
             << "key_frame " << pFrame->key_frame << " [frame], "
             << pCodecContext->bit_rate << " bit_rate [codec context], "
             << pCodecContext->time_base.num << " / " << pCodecContext->time_base.den
             << " time_base [codec context], "
             << pCodecContext->framerate.num << " / " << pCodecContext->framerate.den
             << " framerate [codec context], "
             << pCodecContext->sample_rate << " sample_rate [codec context]";


    int data_size = av_samples_get_buffer_size(nullptr,
                               pCodecContext->channels,
                               pFrame->nb_samples,
                               pCodecContext->sample_fmt,
                               1);
//    uint8_t *audio_buf = new uint8_t[64000];
//    memcpy(audio_buf, pFrame->data[0], data_size);

    QByteArray *byteArray = new QByteArray();;
    byteArray->setRawData((const char *)pFrame->data[0], data_size);
    //writeToAudio((const char *)pFrame->data[0], (qint64)data_size);

    //encode
    {
        AVFrame *inputFrame = av_frame_alloc();
        inputFrame->format = pEncoderCodecContext->sample_fmt;
        inputFrame->channels = pEncoderCodecContext->channels;
        inputFrame->channel_layout = pEncoderCodecContext->channel_layout;
        inputFrame->sample_rate = pEncoderCodecContext->sample_rate;
        inputFrame->nb_samples = 1024;
        av_frame_get_buffer(inputFrame, 1);
    }

    //int q_data_size = byteArray->size();
    //writeToAudio(byteArray->data(), q_data_size);

//    double pTime = (pFrame->pts + 0.0) / (pCodecContext->time_base.den + 0.0);
//    emit decodedFrame(byteArray, pTime);

    return response;
}

std::string FFTest::avErr2str(int errnum)
{
    const int errbuf_size = 64;
    char errbuf[errbuf_size]{0};
    av_strerror(errnum, errbuf, errbuf_size);
    return std::string(errbuf);
}

void FFTest::initAudio(int freq, int channels)
{
    int sampleRate = freq;//44100;
    int channelCount = channels;//2;
    int sampleSize = 32;//16; //bits per sample

    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultOutputDevice();

    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(channelCount);
    format.setSampleSize(sampleSize);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::Float);

    if(!deviceInfo.isFormatSupported(format))
    {
        qDebug() << "Audio format not supported!";
        format = deviceInfo.nearestFormat(format);
    }

    m_audioOutput.reset(new QAudioOutput(deviceInfo, format));
    m_audioOutput->setBufferSize(8192);
    m_audioDevice = m_audioOutput->start();
    int audioBufferSize = m_audioOutput->bufferSize();
    int audioPeriodSize = m_audioOutput->periodSize();
    qDebug() << m_space << Q_FUNC_INFO << " audio buffer size: " << audioBufferSize
             << "; period size: " << audioPeriodSize;
}

void FFTest::writeToAudio(const char *buffer, qint64 length)
{
    int bufferSize = m_audioOutput->bufferSize();
    int bytesFree = m_audioOutput->bytesFree();
    int perSize = m_audioOutput->periodSize();
    int bytesWritten = m_audioDevice->write(buffer, length);

//    m_audioDevice->write(buffer, perSize);
//    m_audioDevice->write(buffer + perSize, length - perSize);

    QThread::currentThread()->msleep(1000.0 * 0.5 * 1024.0 / 44100.0);
}

int FFTest::initEncoder()
{
    qDebug() << m_space << Q_FUNC_INFO;

    int result = 0;

    result = avformat_alloc_output_context2(&pEncoderFormatContext, nullptr, nullptr, m_outputFileName);
    if(result < 0 || pEncoderFormatContext == nullptr)
    {
        qDebug() << m_space << m_space
                 << "could not allocate memory for output format";
        return result;
    }
    qDebug() << m_space << m_space
             << "Allocated memory for output format";

    pEncoderStream = avformat_new_stream(pEncoderFormatContext, nullptr);
    if(pEncoderStream == nullptr)
    {
        qDebug() << m_space << m_space << "Encoder stream not created!";
        return -1;
    }
    qDebug() << m_space << m_space << "Encoder stream created.";

    pEncoderCodec = avcodec_find_encoder_by_name("mp3");
    if(pEncoderCodec == nullptr)
    {
        qDebug() << m_space << m_space << "Could not find the proper codec!";
        return -1;
    }
    qDebug() << m_space << m_space << "Found the proper codec.";

    pEncoderCodecContext = avcodec_alloc_context3(pEncoderCodec);
    if(pEncoderCodecContext == nullptr)
    {
        qDebug() << m_space << m_space << "Could not allocated memory for codec context!";
        return -1;
    }
    qDebug() << m_space << m_space << "Allocated memory for codec context.";

    int OUTPUT_CHANNELS = 2;
    int OUTPUT_BIT_RATE = 196000;
    int sample_rate = 44100;

    pEncoderCodecContext->channels = OUTPUT_CHANNELS;
    pEncoderCodecContext->channel_layout = av_get_default_channel_layout(OUTPUT_CHANNELS);
    pEncoderCodecContext->sample_rate = sample_rate;
    pEncoderCodecContext->sample_fmt = AV_SAMPLE_FMT_FLT;//pEncoderCodec->sample_fmts[0];
    pEncoderCodecContext->bit_rate = OUTPUT_BIT_RATE;
    pEncoderCodecContext->time_base = AVRational{1, sample_rate};

    pEncoderCodecContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    pEncoderStream->time_base = pEncoderCodecContext->time_base;

    result = avcodec_open2(pEncoderCodecContext, pEncoderCodec, nullptr);
    if(result < 0)
    {
        qDebug() << m_space << m_space
                 << "Could not open the codec!";
        return result;
    }

    avcodec_parameters_from_context(pEncoderStream->codecpar, pEncoderCodecContext);

    return result;
}

///////////////////////////////////////////////////////////////////////////////

FFTestOps::FFTestOps(FFTest *ffTest, QObject *parent)
    : QObject(parent)
    , m_ffTest(ffTest)
    , m_lastpTime(0)
{
    this->moveToThread(&m_thread);
    m_timer.moveToThread(&m_thread);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &FFTestOps::onTimer);
    connect(m_ffTest, &FFTest::decodedFrame, this, &FFTestOps::onDecodedFrame);

    m_thread.start();
}

FFTestOps::~FFTestOps()
{
    m_timer.stop();

    m_thread.quit();
    m_thread.wait();
}

void FFTestOps::onInitTimer()
{

}

void FFTestOps::onTimer()
{
    AudioFrame af = m_playQueue.dequeue();
    writeToAudio(af.buffer);
}

void FFTestOps::onDecodedFrame(QByteArray *buffer, double ptime)
{
    if(ptime == 0)
    {
        writeToAudio(buffer);
        return;
    }
    AudioFrame af;
    af.buffer = buffer;
    af.pTime = ptime;
    m_playQueue.enqueue(af);
    //m_timer.start(1000.0 * (ptime - m_lastpTime));
    //QTimer::singleShot(1000.0 * (ptime - m_lastpTime), this, &FFTestOps::onTimer);
    //m_thread.msleep(1000.0 * (ptime - m_lastpTime));
    //onTimer();
    m_lastpTime = ptime;
}

void FFTestOps::writeToAudio(QByteArray *buffer)
{
    m_ffTest->m_audioDevice->write(buffer->data(), buffer->length());
    //delete buffer;
}
