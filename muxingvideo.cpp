#include "muxingvideo.h"

MuxingVideo::MuxingVideo()
{
    //  av_register_all();
}

MuxingVideo::~MuxingVideo()
{
    release();
//     if (ret < 0 && ret != AVERROR_EOF)
//     {
//             printf( "Error occurred.\n");
//             return ;
//     }
}

void MuxingVideo::release()
{
    if (nullptr != m_outformt_ctx && !(m_outformt_ctx->oformat->flags & AVFMT_NOFILE))
    {
        avio_close(m_outformt_ctx->pb);
    }
    if (nullptr != m_outformt_ctx)
    {
        avformat_free_context(m_outformt_ctx);
    }
    if (nullptr !=m_codec_ctx )
    {
        avcodec_free_context(&m_codec_ctx);
    }
    
     m_outformt_ctx = nullptr;

}

void MuxingVideo::finished()
{
 
    av_write_trailer(m_outformt_ctx);
    release();
    m_frame_index = 0;
}


void MuxingVideo::WriteFrame(uint8_t* data, uint32_t size)
{
    AVPacket pkt;
    if (nullptr != data)
    {
        pkt.size = size;
        pkt.data = data;
        pkt.flags = 0x01;
        pkt.stream_index = 0;
        pkt.duration = AV_TIME_BASE*10/20000;
        pkt.dts = m_frame_index*pkt.duration;
        pkt.pts = pkt.dts;
        m_frame_index++;

        // av_packet_rescale_ts(&pkt, m_codec_ctx->time_base, out_stream->time_base);
        // pkt.stream_index = out_stream->index;
        if (av_write_frame(m_outformt_ctx, &pkt) < 0) {
                printf( "Error muxing packet\n");
            //    return -1;
        }
    }
    
}

int MuxingVideo::InitSaveStreamer(const char* filename,uint8_t *data,uint32_t size,FrameInfo frameinfo)
{
    m_frame_index = 0;
    avformat_alloc_output_context2(&m_outformt_ctx,NULL,"mp4",NULL);
    if(!m_outformt_ctx)
    {
        fprintf(stderr, "Could not create output context\n");
        return -1;
    }
    
    auto  out_stream = avformat_new_stream(m_outformt_ctx,NULL);
    if(! out_stream)
    {
        printf("Failed allocating output stream!\n");
      //  goto end;
    }

    out_stream->id = (int) (m_outformt_ctx->nb_streams - 1);

    m_codec_ctx =  avcodec_alloc_context3(nullptr);

    //auto  m_codec_ctx = out_stream->codec;
    m_codec_ctx->codec_id = AV_CODEC_ID_H264;
    m_codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    m_codec_ctx->codec_tag = 0;
    m_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_codec_ctx->width = frameinfo.width;
    m_codec_ctx->height = frameinfo.height;
    m_codec_ctx->extradata = data;
    m_codec_ctx->extradata_size = size;
    out_stream->time_base.num = 1;
    out_stream->time_base.den = frameinfo.fps;

    m_codec_ctx->time_base.num = 1;
    m_codec_ctx->time_base.den = frameinfo.fps;
            
    av_dump_format(m_outformt_ctx,0,filename,1);

    auto ret = avcodec_parameters_from_context(out_stream->codecpar, m_codec_ctx);
    if (ret < 0) {
        // av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream #%u\n", i);
        printf( "Failed to copy encoder parameters to output stream");
        return ret;
    }


    if (!(m_outformt_ctx->oformat->flags & AVFMT_NOFILE)) 
    {
           auto  ret = avio_open(&m_outformt_ctx->pb, filename, AVIO_FLAG_WRITE);
            if (ret < 0) 
            {
                printf( "Could not open output URL '%s'", filename);
                return -2;
            }
    }

    //写文件头（Write file header）
     ret = avformat_write_header(m_outformt_ctx, NULL);
    if (ret < 0) 
    {
        printf( "Error occurred when opening output URL\n");
        return -3;
    }
    return 0;

 
}