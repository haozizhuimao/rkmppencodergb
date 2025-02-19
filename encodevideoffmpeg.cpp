#include "../encode/encodevideoffmpeg.h"
#include <iostream>


FFMPEGEncodeVideo::FFMPEGEncodeVideo(/* args */)
{
}

FFMPEGEncodeVideo::~FFMPEGEncodeVideo()
{
    Release();
}

int FFMPEGEncodeVideo::Initencoder(InputInfo& encoderinfo,int srcindex,std::function<void(int,AVPacket&,int)>  writefilecallback)
{
    m_srcindex = srcindex;
    m_writefilecallback = writefilecallback;
    if ( AV_CODEC_ID_H264 == (AVCodecID)encoderinfo.avcodeid){
        m_encodecodec =  const_cast<AVCodec*>(avcodec_find_encoder_by_name(encoderinfo.codecname));
        if (nullptr == m_encodecodec) {
            m_encodecodec = const_cast<AVCodec*>(avcodec_find_encoder((AVCodecID)encoderinfo.avcodeid));
            if (nullptr == m_encodecodec) {
                XQTKLOG_LOGGER_ERROR(xqtklog::getptr(),"Could not find encoder for :%s",avcodec_get_name((AVCodecID)encoderinfo.avcodeid));
                return -1;
            }
        }
    }else{
        m_encodecodec = const_cast<AVCodec*>(avcodec_find_encoder((AVCodecID)encoderinfo.avcodeid));
        if (nullptr == m_encodecodec) {
            XQTKLOG_LOGGER_ERROR(xqtklog::getptr(),"Could not find encoder for 2:%s",avcodec_get_name((AVCodecID)encoderinfo.avcodeid));
            return -1;
        }
    }

    m_codecContext = avcodec_alloc_context3(m_encodecodec);
    if (nullptr ==m_codecContext) {
        XQTKLOG_LOGGER_ERROR(xqtklog::getptr(),"Could not alloc an encoding context");
        return -1;
    }
    switch ((m_encodecodec)->type) {
    case AVMEDIA_TYPE_VIDEO:{
        m_codecContext->codec_id = (AVCodecID)encoderinfo.avcodeid;
       // m_codecContext->bit_rate = 400000;
        m_codecContext->width    = encoderinfo.width;
        m_codecContext->height   = encoderinfo.height;
        // m_codecContext->time_base = (AVRational){1, encoderinfo.fps};
        m_codecContext->time_base.den = encoderinfo.fps;
        m_codecContext->time_base.num = 1;
        // m_codecContext->framerate = (AVRational){encoderinfo.fps, 1};

        m_codecContext->gop_size      = encoderinfo.fps / 2; /* emit one intra frame every twelve frames at most */
        m_codecContext->pix_fmt       = (AVPixelFormat)encoderinfo.pixfmt;
        if (m_codecContext->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        //     /* just for testing, we also add B-frames */
         m_codecContext->max_b_frames = 2;
         }
        break;
    }
    default:
        break;
    }
    OpenVideo(nullptr);
    m_frameindex = 0;
    m_initialized = true;

    XQTKLOG_LOGGER_DEBUG(xqtklog::getptr(),"FFMPEGEncodeVideo Initecnoder successed!");
    return 0;
}

void FFMPEGEncodeVideo::OpenVideo(AVDictionary *opt_arg)
{
    AVDictionary *opt = NULL;
    av_dict_copy(&opt, opt_arg, 0);

    /* open the codec */
    int ret = avcodec_open2(m_codecContext, m_encodecodec, &opt);
   // int ret = avcodec_open2(m_codecContext, m_encodecodec, nullptr);
    av_dict_free(&opt);
    if (ret < 0) {
       XQTKLOG_LOGGER_ERROR(xqtklog::getptr(), "Could not open video codec: %d", ret);
        return;
    }
    m_avframe = av_frame_alloc();
    if ( nullptr == m_avframe){
        return ;
    }
    m_avframe->format = m_codecContext->pix_fmt;
    m_avframe->width  = m_codecContext->width;
    m_avframe->height = m_codecContext->height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(m_avframe, 0);
    if (ret < 0) {
        XQTKLOG_LOGGER_ERROR(xqtklog::getptr(), "Could not allocate frame data");
        return ;
    }
}

int FFMPEGEncodeVideo::WriteRGBData(const uint8_t *rgb,int size,int srcpixfmt)
{
    std::lock_guard<std::mutex> lock(m_write_mutex);  
  //  std::cout << "FFMPEGEncodeVideo::WriteRGBData step1\r\n";
    if (!m_initialized ) {
        XQTKLOG_LOGGER_ERROR(xqtklog::getptr(), "video is not started");
        return -1;
    }
    auto ret = av_frame_make_writable(m_avframe);
    if (ret < 0) {
        XQTKLOG_LOGGER_ERROR(xqtklog::getptr(), "frame not writable");
        return -1;
    }
   
    if(nullptr == m_swscontext){
        InitSwsContext((enum AVPixelFormat)srcpixfmt);
    }
 
    if (nullptr == rgb || nullptr == m_codecContext) {    
    //    std::cout << "data is nullptr\r\n";
        return -1;
    }
   
    if (AV_PIX_FMT_YUV420P == m_codecContext->pix_fmt){
        const int in_linesize[1] = {m_avframe->width * 3};
               sws_scale(m_swscontext,
            &rgb, in_linesize, 0, m_avframe->height,  // src
            m_avframe->data, m_avframe->linesize); // dst
    }else {
    
        sws_scale(m_swscontext,
             &rgb, m_avframe->linesize, 0, m_avframe->height,  // src
             m_avframe->data, m_avframe->linesize); // dst
    }
    
    m_avframe->pts = m_frameindex++;
    
    encode(m_avframe);
 //   std::cout << "FFMPEGEncodeVideo WriteRGBData step5\r\n";
    return 0;
}

int FFMPEGEncodeVideo::InitSwsContext(enum AVPixelFormat srcpixfmat)
{
    m_swscontext = sws_getContext(
            m_avframe->width , m_avframe->height , srcpixfmat,   // src
            m_avframe->width , m_avframe->height , m_codecContext->pix_fmt, // dst
            SWS_BICUBIC, NULL, NULL, NULL
    );
    if (nullptr == m_swscontext) {
        XQTKLOG_LOGGER_ERROR(xqtklog::getptr(), "could not initialize the conversion context");
        // goto cleanup_name_context_stream_context_frame;
        return -1;
    }
    return 0;
}

 void FFMPEGEncodeVideo::EndEncode()
 {
    Release();
 }

void FFMPEGEncodeVideo::Release()
{
    std::lock_guard<std::mutex> lock(m_write_mutex);  
    if (nullptr != m_swscontext){
        sws_freeContext(m_swscontext);
        m_swscontext = nullptr;
    }

    if (nullptr != m_avframe){
        av_frame_free(&m_avframe);
        m_avframe = nullptr;
    }

    if (nullptr != m_codecContext){
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
    }

    if (nullptr != m_codecContext){
        avcodec_close(m_codecContext);
        m_codecContext = nullptr;
    }
    m_initialized = false;
}

void FFMPEGEncodeVideo::encode(const AVFrame* frame)
{
    int   ret = avcodec_send_frame(m_codecContext, m_avframe);
    if (ret < 0) {
        XQTKLOG_LOGGER_ERROR(xqtklog::getptr(), "error sending a frame for encoding");
        return ;
    }

    do {
        AVPacket pkt = {0};
        ret = avcodec_receive_packet(m_codecContext, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            // fprintf(stderr, "error encoding a frame: %s\n", av_err2str(ret));
            XQTKLOG_LOGGER_ERROR(xqtklog::getptr(), "error encoding a frame: %d", ret);
            return ;
        }
        if (nullptr !=  m_writefilecallback) {
            m_writefilecallback(m_srcindex,pkt,0);
        }
         av_packet_unref(&pkt);        
    } while (ret >= 0);
}