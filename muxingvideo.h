#pragma once
#include "commoninfo.h"

extern "C"
{
	#include <libavformat/avformat.h>
};
 

class MuxingVideo
{
private:
    
    AVFormatContext * m_outformt_ctx = nullptr;
    AVCodecContext *m_codec_ctx = nullptr;
    // AVStream *m_out_stream = nullptr ;
    int m_frame_index=0;
private:
    void release();
public:
    MuxingVideo();
    ~MuxingVideo();

    void WriteFrame(uint8_t* data, uint32_t size);
    int InitSaveStreamer(const char* stream,uint8_t *data,uint32_t size,FrameInfo frameinfo);
    void finished();
};

