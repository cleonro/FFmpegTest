#include "fftest.h"

#include <QThread>
#include <QMutexLocker>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QByteArray>
#include <QDebug>
#include <QtGlobal>

#include <iostream>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    //#include <libswscale/swscale.h>

    #include <libswresample/swresample.h>

    #include <libavutil/audio_fifo.h>

    #include <libavutil/opt.h>
}

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

FFTest::FFTest(QObject *parent)
    : QObject(parent)
    , m_stop(false)
    , m_space("    ")
    , m_ffTestOps(this)

    , m_sendToAudio(false)
    , m_encode(false)

    , m_audioDevice(nullptr)
    , pFormatContext(nullptr)
    , pCodec(nullptr)
    , pCodecParameters(nullptr)
    , pCodecContext(nullptr)
    //, m_outputFileName("FFTest.mp3")
    //, m_outputFileName("FFTest.ac3")
    //, m_outputFileName("FFTest.m4a")
    //, m_outputFileName("FFTest.aac")
    //, m_outputFileName("FFTest")
    , m_outputFileName("FFTest.adts")
    , pEncoderFormatContext(nullptr)
    , pEncoderStream(nullptr)
    , pEncoderCodec(nullptr)
    , pEncoderCodecContext(nullptr)
    , pSwrContext(nullptr)
    , pAudioFifo(nullptr)
    , pConvertedSamples(nullptr)
    , m_convertedSamplesInitialized(false)
    , m_encodePTS(0)
{
    this->moveToThread(&m_thread);
    connect(this, &FFTest::openSignal, this, &FFTest::open);

    m_thread.start();
}

bool FFTest::setEncode(bool encode)
{
    QMutexLocker lock(&m_mutex);
    m_encode = encode;
}

bool FFTest::setSendToAudio(bool sendToAudio)
{
    QMutexLocker lock(&m_mutex);
    m_sendToAudio = sendToAudio;
}

FFTest::~FFTest()
{
    m_stop = true;
    m_thread.quit();
    m_thread.wait();

    clear();
}

void FFTest::stop()
{
    QMutexLocker lock(&m_mutex);
    m_stop = true;
}

bool FFTest::needToStop()
{
    QMutexLocker lock(&m_mutex);
    static int count = 0;
    if(++count > 1000)
    {
        m_stop = true;
    }
    return m_stop;
}

void FFTest::openRequest(QString filePath)
{
    qDebug() << Q_FUNC_INFO << " -> current thread: " << QThread::currentThread();
    emit openSignal(filePath);
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

    if(m_convertedSamplesInitialized)
    {
        av_freep(pConvertedSamples);
        delete [] pConvertedSamples;
        m_convertedSamplesInitialized = false;
    }

    m_encodePTS = 0;
}

void FFTest::init()
{
    clear();

    qDebug() << Q_FUNC_INFO;

    unsigned int avFormatVersion = avformat_version();
    unsigned int avUtilVersion = avutil_version();
    unsigned int avCodecVersion = avcodec_version();

    unsigned int a = (avFormatVersion & 0xFF); //extract first byte
    unsigned int b = ((avFormatVersion >> 8) & 0xFF); //extract second byte
    unsigned int c = ((avFormatVersion >> 16) & 0xFF); //extract third byte
    unsigned int d = ((avFormatVersion >> 24) & 0xFF); //extract fourth byte
    qDebug() << m_space << avformat_license();
    qDebug() << m_space << "avformat version: " << avFormatVersion << " - "
             << d << "."
             << c << "."
             << b << "."
             << a;

    a = (avUtilVersion & 0xFF); //extract first byte
    b = ((avUtilVersion >> 8) & 0xFF); //extract second byte
    c = ((avUtilVersion >> 16) & 0xFF); //extract third byte
    d = ((avUtilVersion >> 24) & 0xFF); //extract fourth byte
    qDebug() << m_space << "avutil version: " << avUtilVersion << " - "
             << d << "."
             << c << "."
             << b << "."
             << a;

    a = (avCodecVersion & 0xFF); //extract first byte
    b = ((avCodecVersion >> 8) & 0xFF); //extract second byte
    c = ((avCodecVersion >> 16) & 0xFF); //extract third byte
    d = ((avCodecVersion >> 24) & 0xFF); //extract fourth byte
    qDebug() << m_space << "avcodec version: " << avCodecVersion << " - "
             << d << "."
             << c << "."
             << b << "."
             << a;

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

void FFTest::open(QString filePath)
{
    int audioFreq = 0;
    int audioChannels = 0;

    clear();

    if(filePath.isEmpty())
    {
        //MP3 stream
        //filePath = "http://radiorivendell.ddns.net:8000/128kbit.mp3";
        filePath = "http://radiorivendell.ddns.net:8000/128kbit.mp3";

        //AAC stream
        //filePath = "http://91.220.63.165:8048/live.aac";
    }

    qDebug() << Q_FUNC_INFO << " -> current thread: " << QThread::currentThread()
             << "; working thread: " << &m_thread;
    int status = avformat_open_input(&pFormatContext,
                                     filePath.toStdString().c_str(),
                                     nullptr, nullptr);
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

    m_audio_stream_index = -1;
    m_video_stream_index = -1;

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
            if(m_audio_stream_index == -1)
            {
                m_audio_stream_index = i;
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
    initResampler();
    initFifo();

    int count = 0;

    //initialize sbr collecting callback
    hasSBR3(pCodecContext);
    //

    while(av_read_frame(pFormatContext, pPacket) >= 0 && !needToStop())
    {

        int packet_stream_index = pPacket->stream_index;
        if(packet_stream_index == m_audio_stream_index)
        //if(packet_stream_index == video_stream_index)
        {
            qDebug() << m_space << m_space
                     << "AVPacket::pts " << pPacket->pts;

//            response = decode_packet(pPacket, pCodecContext, pFrame);
//            qDebug() << m_space << m_space << m_space
//                     << "decode_packet response: " << response;

//            qDebug() << m_space << m_space
//                     << "SBR Analysis before avcodec_send_packet:";
//            qDebug() << m_space << m_space << m_space << m_space
//                     << "hasSBR1 -> " << hasSBR1(pCodecContext) <<"; "
//                     << "hasSBR2 -> " << hasSBR2(pCodecContext);

            std::cout << ++count << ". Packet will be sent: ";
            response = avcodec_send_packet(pCodecContext, pPacket);
            std::cout << ", final sbr result " << hasSBR3(pCodecContext) << std::endl;

//            qDebug() << m_space << m_space
//                     << "SBR Analysis after avcodec_send_packet:";
//            qDebug() << m_space << m_space << m_space << m_space
//                     << "hasSBR1 -> " << hasSBR1(pCodecContext) <<"; "
//                     << "hasSBR2 -> " << hasSBR2(pCodecContext);

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

    //?
    if(m_encode)
    {
        checkCloseFifo();
    }

    //encoder
    {
        av_write_trailer(pEncoderFormatContext);
        //?
        avio_closep(&pEncoderFormatContext->pb);
        //
        //?
        //avformat_close_input(&pEncoderFormatContext);

        avformat_close_input(&pEncoderFormatContext);
        avformat_free_context(pEncoderFormatContext);
        avcodec_free_context(&pEncoderCodecContext);
    }
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

    qDebug() << m_space << m_space
             << "Frame " << pCodecContext->frame_number << " [continuation - data size]: "
             << "frame.nb_sample " << pFrame->nb_samples << "; "
             << " Format " << av_get_sample_fmt_name(pCodecContext->sample_fmt) << "; "
             << " Format is planar " << av_sample_fmt_is_planar(pCodecContext->sample_fmt) << "; "
             << " Channels " << pCodecContext->channels << "; "
             << " Codec frame size " << pCodecContext->frame_size << "; "
             << "Bytes per sample " << av_get_bytes_per_sample(pCodecContext->sample_fmt) << "; "
             << " Buffer size " << data_size;

    if(m_sendToAudio)
    {
        QByteArray *byteArray = new QByteArray();;
        //byteArray->setRawData((const char *)pFrame->data[0], data_size);
        byteArray->setRawData((const char *)pFrame->extended_data[0], data_size);
        byteArray->setRawData((const char *)pFrame->extended_data[0], data_size / 2);
        byteArray->reserve(data_size);
        for(int i = 0; i < data_size / 2; ++i)
        {
            byteArray->push_back(pFrame->data[0][i] - 127);
            byteArray->push_back(pFrame->data[1][i] - 127);
        }
        int q_data_size = byteArray->size();
        writeToAudio(byteArray->data(), q_data_size);
        //double pTime = (pFrame->pts + 0.0) / (pCodecContext->time_base.den + 0.0);
        //emit decodedFrame(byteArray, pTime);

    }

    if(m_encode)
    {
//        AVFrame *inputFrame = av_frame_alloc();
//        inputFrame->format = pEncoderCodecContext->sample_fmt;
//        inputFrame->channels = pEncoderCodecContext->channels;
//        inputFrame->channel_layout = pEncoderCodecContext->channel_layout;
//        inputFrame->sample_rate = pEncoderCodecContext->sample_rate;
//        inputFrame->nb_samples = 1024;
//        av_frame_get_buffer(inputFrame, 0);

        bool continueEncode = true;

        if(!m_convertedSamplesInitialized)
        {
            pConvertedSamples = new uint8_t*[pEncoderCodecContext->channels];
            int allocatedSamples = av_samples_alloc(pConvertedSamples, nullptr, pEncoderCodecContext->channels,
                                                    2048/*pFrame->nb_samples*/, pEncoderCodecContext->sample_fmt, 0);
            qDebug() << m_space << m_space
                     << "Allocated space for " << allocatedSamples << " converted samples!";
            m_convertedSamplesInitialized = true;
        }

        int audioFifoSize0 = av_audio_fifo_size(pAudioFifo);
        //if(av_audio_fifo_size(pAudioFifo) < pEncoderCodecContext->frame_size)
        //{
            int error = swr_convert(pSwrContext,
                        pConvertedSamples, pFrame->nb_samples,
                        (const uint8_t**)pFrame->extended_data, pFrame->nb_samples);
            if(error < 0)
            {
                qDebug() << m_space << m_space
                         << "Could not convert decoded frame!";
                continueEncode = false;
            }
            else
            {
                qDebug() << m_space << m_space
                         << "Converted decoded frame!";

                try
                {
                    error = av_audio_fifo_realloc(pAudioFifo, audioFifoSize0 + pFrame->nb_samples);
                }
                catch(std::exception &e)
                {
                    std::cout << "av_audio_fifo_realloc EXCEPTION: " << e.what() << std::endl;
                }
                if(error < 0)
                {
                    qDebug() << m_space << m_space
                             << "Could not alloc space to store decoded frame!";
                }
                else
                {
                    qDebug() << m_space << m_space
                             << "Allocated space to Store decoded frame!";
                }
                int writesize = av_audio_fifo_write(pAudioFifo, (void**)pConvertedSamples, pFrame->nb_samples);
                if(writesize < pFrame->nb_samples)
                {

                }
            }
        //}

        int trials = 0;
        int audioFifoSize = av_audio_fifo_size(pAudioFifo);
        while(audioFifoSize >= pEncoderCodecContext->frame_size)
        {
            //1. alloc output frame
            //const int framesize = FFMIN(av_audio_fifo_size(pAudioFifo), pEncoderCodecContext->frame_size);
            const int framesize = pEncoderCodecContext->frame_size;
            AVFrame *output_frame = av_frame_alloc();
            if(output_frame == nullptr)
            {
                qDebug() << m_space << m_space
                         << "output frame not allocated!";
                if(++trials < 10)
                {
                    continue;
                }
                break;
            }
            output_frame->nb_samples = framesize;
            output_frame->channel_layout = pEncoderCodecContext->channel_layout;
            output_frame->format = pEncoderCodecContext->sample_fmt;
            output_frame->sample_rate = pEncoderCodecContext->sample_rate;
            //?
            output_frame->pts = pEncoderCodecContext->initial_padding + m_encodePTS;
            m_encodePTS += output_frame->nb_samples;
            //

            int error = av_frame_get_buffer(output_frame, 0);
            if(error < 0)
            {
                qDebug() << m_space << m_space
                         << "output frame buffers not allocated!";
                if(++trials < 10)
                {
                    continue;
                }
                break;
            }

            //2. read output frame from audio fifo
            int readsize = av_audio_fifo_read(pAudioFifo, (void**)output_frame->data, framesize);
            if(readsize < framesize)
            {
                qDebug() << m_space << m_space
                         << "Couldn't read from audio fifo!";
                if(++trials < 10)
                {
                    continue;
                }
                break;
            }
            audioFifoSize = av_audio_fifo_size(pAudioFifo);

            //3. encode output frame
            AVPacket *outputPacket = av_packet_alloc();
            outputPacket->data = nullptr;
            outputPacket->size = 0;
            response = avcodec_send_frame(pEncoderCodecContext, output_frame);
            while(response >= 0)
            {
                response = avcodec_receive_packet(pEncoderCodecContext, outputPacket);
                if(response == AVERROR(EAGAIN) || response == AVERROR_EOF)
                {
                    break;
                }
                else if(response < 0)
                {
                    qDebug() << m_space << m_space
                             << "Error while receiving packet from encoder: "
                             << avErr2str(response).c_str();
                    return -1;
                }

                outputPacket->stream_index = 0;//m_audio_stream_index;
                av_packet_rescale_ts(outputPacket, pEncoderCodecContext->time_base, pCodecContext->time_base);
                //response = av_interleaved_write_frame(pEncoderFormatContext, outputPacket);
                response = av_write_frame(pEncoderFormatContext, outputPacket);
                if(response < 0)
                {
                    qDebug() << m_space << m_space
                             << "Error " << response << " while receiving packet from decoder: "
                             << avErr2str(response).c_str();
                    return -1;
                }
            }
            av_packet_unref(outputPacket);
            av_packet_free(&outputPacket);

        }
    }

    return response;
}

void FFTest::checkCloseFifo()
{
    int response = 0;

    int trials = 0;
    int audioFifoSize = av_audio_fifo_size(pAudioFifo);
    while(audioFifoSize > 0)
    {
        //1. alloc output frame
        //const int framesize = FFMIN(av_audio_fifo_size(pAudioFifo), pEncoderCodecContext->frame_size);
        const int framesize = pEncoderCodecContext->frame_size;
        AVFrame *output_frame = av_frame_alloc();
        if(output_frame == nullptr)
        {
            qDebug() << m_space << m_space
                     << "FFTest::checkCloseFifo - output frame not allocated!";
            if(++trials < 10)
            {
                continue;
            }
            break;
        }
        output_frame->nb_samples = framesize;
        output_frame->channel_layout = pEncoderCodecContext->channel_layout;
        output_frame->format = pEncoderCodecContext->sample_fmt;
        output_frame->sample_rate = pEncoderCodecContext->sample_rate;
        //?
        output_frame->pts = pEncoderCodecContext->initial_padding + m_encodePTS;
        m_encodePTS += output_frame->nb_samples;
        //

        int error = av_frame_get_buffer(output_frame, 0);
        if(error < 0)
        {
            qDebug() << m_space << m_space
                     << "output frame buffers not allocated!";
            if(++trials < 10)
            {
                continue;
            }
            break;
        }

        //2. read output frame from audio fifo
        int readsize = av_audio_fifo_read(pAudioFifo, (void**)output_frame->data, framesize);
        if(readsize <= 0)//< framesize)
        {
            qDebug() << m_space << m_space
                     << "Couldn't read from audio fifo!";
            if(++trials < 10)
            {
                continue;
            }
            break;
        }
        audioFifoSize = av_audio_fifo_size(pAudioFifo);

        //3. encode output frame
        AVPacket *outputPacket = av_packet_alloc();
        outputPacket->data = nullptr;
        outputPacket->size = 0;
        response = avcodec_send_frame(pEncoderCodecContext, output_frame);
        while(response >= 0)
        {
            response = avcodec_receive_packet(pEncoderCodecContext, outputPacket);
            if(response == AVERROR(EAGAIN) || response == AVERROR_EOF)
            {
                break;
            }
            else if(response < 0)
            {
                qDebug() << m_space << m_space
                         << "Error while receiving packet from encoder: "
                         << avErr2str(response).c_str();
                return;// -1;
            }

            outputPacket->stream_index = 0;//m_audio_stream_index;
            av_packet_rescale_ts(outputPacket, pEncoderCodecContext->time_base, pCodecContext->time_base);
            //response = av_interleaved_write_frame(pEncoderFormatContext, outputPacket);
            response = av_write_frame(pEncoderFormatContext, outputPacket);
            if(response < 0)
            {
                qDebug() << m_space << m_space
                         << "Error " << response << " while receiving packet from decoder: "
                         << avErr2str(response).c_str();
                return;// -1;
            }
        }
        av_packet_unref(outputPacket);
        av_packet_free(&outputPacket);
    }
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
    //format.setSampleType(QAudioFormat::UnSignedInt);

    if(!deviceInfo.isFormatSupported(format))
    {
        qDebug() << "Audio format not supported!";
        format = deviceInfo.nearestFormat(format);
    }

    m_audioOutput.reset(new QAudioOutput(deviceInfo, format));
    m_audioOutput->setBufferSize(100000);//8192);
    m_audioDevice = m_audioOutput->start();
    int audioBufferSize = m_audioOutput->bufferSize();
    int audioPeriodSize = m_audioOutput->periodSize();
    qDebug() << m_space << Q_FUNC_INFO << " audio buffer size: " << audioBufferSize
             << "; period size: " << audioPeriodSize;
}

void FFTest::writeToAudio(const char *buffer, qint64 length)
{
    int bufferSize = m_audioOutput->bufferSize();
    int bytesFree = m_audioOutput->bytesFree();//m_audioOutput->bufferSize() - m_audioOutput->bytesFree();
    int perSize = m_audioOutput->periodSize();
    if(bytesFree >= length)
    {
        int bytesWritten = m_audioDevice->write(buffer, length);
        qDebug() << "audio data written " << bytesWritten;
    }
    else while(bytesFree < length)
    {
        QThread::currentThread()->msleep(500);
        bytesFree = m_audioOutput->bytesFree();//m_audioOutput->bufferSize() - m_audioOutput->bytesFree();
        qDebug() << "audio data not written__ " << bytesFree;
    }
    int bytesWritten = m_audioDevice->write(buffer, length);
    qDebug() << "audio data written " << bytesWritten;

//    m_audioDevice->write(buffer, perSize);
//    m_audioDevice->write(buffer + perSize, length - perSize);

    //QThread::currentThread()->msleep(1000.0 * 0.5 * 1024.0 / 44100.0);
}

int FFTest::initEncoder()
{
    qDebug() << m_space << Q_FUNC_INFO;

    int result = 0;

    result = avformat_alloc_output_context2(&pEncoderFormatContext,
                                            nullptr,
                                            nullptr,
                                            //"rtp://127.0.0.1:9003");
                                            m_outputFileName);
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

//    pEncoderCodec = avcodec_find_encoder_by_name("ac3");
//    if(pEncoderCodec == nullptr)
//    {
//        pEncoderCodec = avcodec_find_encoder(AV_CODEC_ID_AC3);
//    }
    //pEncoderCodec = avcodec_find_encoder_by_name("aac");
    pEncoderCodec = avcodec_find_encoder_by_name("libfdk_aac");
    if(pEncoderCodec == nullptr)
    {
        pEncoderCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
        //pEncoderCodec = avcodec_find_encoder(pCodecContext->codec_id);
        //ONLY for test!
        ////pEncoderCodec = pCodec;

        ///pEncoderCodec = avcodec_find_encoder(AV_CODEC_ID_MP3);
    }
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
    int OUTPUT_BIT_RATE = 96000;
    int sample_rate = pCodecContext->sample_rate;//44100;

    pEncoderCodecContext->channels = OUTPUT_CHANNELS;
    pEncoderCodecContext->channel_layout = av_get_default_channel_layout(OUTPUT_CHANNELS);
    pEncoderCodecContext->sample_rate = sample_rate;
    pEncoderCodecContext->sample_fmt = pEncoderCodec->sample_fmts[0];//pCodec->sample_fmts[0];//AV_SAMPLE_FMT_FLT;//pEncoderCodec->sample_fmts[0];
    pEncoderCodecContext->bit_rate = OUTPUT_BIT_RATE;
    pEncoderCodecContext->time_base = AVRational{1, sample_rate};

    //test
    pEncoderCodecContext->profile = FF_PROFILE_AAC_HE_V2;
    //

    pEncoderCodecContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    pEncoderStream->time_base = pEncoderCodecContext->time_base;

    //test - NOT working
//    result = av_opt_set(pEncoderCodecContext->priv_data, "profile", "aac_he", 0);
//    if(result < 0)
//    {
//        qDebug() << m_space << m_space
//                 << "Profile aac_he not set: " << avErr2str(result).c_str();
//    }
//    qDebug() << m_space << m_space
//             << "Profile aac_he set!";
    //

    result = avcodec_open2(pEncoderCodecContext, pEncoderCodec, nullptr);
    //? - is it correct? - maybe works for AAC [NO, it doesn't!]
    //pEncoderCodecContext->frame_size = 2048;//1024;

    if(result < 0)
    {
        qDebug() << m_space << m_space
                 << "Could not open the codec: " << avErr2str(result).c_str();
        return result;
    }

    avcodec_parameters_from_context(pEncoderStream->codecpar, pEncoderCodecContext);

    //file
    if (pEncoderFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
    {
       pEncoderFormatContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    result = avio_open(&pEncoderFormatContext->pb, m_outputFileName, AVIO_FLAG_WRITE);
    if(result < 0)
    {
        qDebug() << m_space << m_space
                 << "Could not open the output file: " << avErr2str(result).c_str();
        return -1;
    }

    AVDictionary* muxer_opts = nullptr;

//    if (sp.muxer_opt_key && sp.muxer_opt_value)
//    {
//        av_dict_set(&muxer_opts, sp.muxer_opt_key, sp.muxer_opt_value, 0);
//    }

//    result = av_dict_copy(&muxer_opts, pEncoderFormatContext->metadata, 0);
//    if(result < 0)
//    {
//        qDebug() << m_space << m_space
//                 << "Error when using AVDictionary!";
//    }


    //result = avformat_write_header(pEncoderFormatContext, &muxer_opts);
    //result = avformat_write_header(pEncoderFormatContext, &pEncoderFormatContext->metadata);
    result = avformat_write_header(pEncoderFormatContext, nullptr);
    if(result < 0)
    {
        qDebug() << m_space << m_space
                 << "An error occurred when opening output file";
        return -1;
    }
    qDebug() << m_space << m_space
             << "Opened output file.";

    return result;
}

int FFTest::initResampler()
{
    qDebug() << m_space << Q_FUNC_INFO;

    int result = 0;

    pSwrContext = swr_alloc_set_opts(nullptr,
                                     av_get_default_channel_layout(pEncoderCodecContext->channels),
                                     pEncoderCodecContext->sample_fmt,
                                     pEncoderCodecContext->sample_rate,

                                     av_get_default_channel_layout(pCodecContext->channels),
                                     pCodecContext->sample_fmt,
                                     pCodecContext->sample_rate,
                                     0,
                                     nullptr);
    if(pSwrContext == nullptr)
    {
        qDebug() << m_space << m_space
                 << "Could not allocate resample context!";
        return AVERROR(ENOMEM);
    }
    qDebug() << m_space << m_space
             << "Allocated resample context!";
    Q_ASSERT(pEncoderCodecContext->sample_rate == pCodecContext->sample_rate);

    result = swr_init(pSwrContext);
    if(result < 0)
    {
        qDebug() << m_space << m_space
                 << "Could not open resample context!";
        swr_free(&pSwrContext);
        return result;
    }
    qDebug() << m_space << m_space
             << "Opened resample context!";

    return result;
}

int FFTest::initFifo()
{
    qDebug() << m_space << Q_FUNC_INFO;

    int result = 0;

    pAudioFifo = av_audio_fifo_alloc(pEncoderCodecContext->sample_fmt, pEncoderCodecContext->channels, 1);
    if(pAudioFifo == nullptr)
    {
        qDebug() << m_space << m_space
                 << "Could not allocate FIFO!";
        return AVERROR(ENOMEM);
    }

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
//    connect(&m_timer, &QTimer::timeout, this, &FFTestOps::onTimer);
    connect(m_ffTest, &FFTest::decodedFrame, this, &FFTestOps::onDecodedFrame);

    m_thread.start();
}

FFTestOps::~FFTestOps()
{
    m_timer.stop();

    m_thread.quit();
    m_thread.wait();
}

//void FFTestOps::onInitTimer()
//{

//}

//void FFTestOps::onTimer()
//{
//    AudioFrame af = m_playQueue.dequeue();
//    writeToAudio(af.buffer);
//}

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
