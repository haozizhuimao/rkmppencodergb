#pragma once
#include "../encode/encodevideo.h"

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/hwcontext.h>
    #include "libavutil/opt.h"
	#include "libavutil/channel_layout.h"
	#include "libavutil/common.h"
	#include "libavutil/imgutils.h"
	#include "libavutil/mathematics.h"
	#include "libavutil/samplefmt.h"
	#include "libavutil/time.h"
	#include "libavutil/fifo.h"
	#include "libavcodec/avcodec.h"
// 	#include "libavcodec/qsv.h"
	#include "libavformat/avformat.h"
//	#include "libavformat/url.h"
	#include "libavformat/avio.h"
//	#include "libavfilter/avcodec.h"
	// #include "libavfilter/avfiltergraph.h"
	#include "libavfilter/avfilter.h"
	#include "libavfilter/buffersink.h"
	#include "libavfilter/buffersrc.h"
	#include "libswscale/swscale.h"
	#include "libswresample/swresample.h"
	#include "libavdevice/avdevice.h"
}


class FFMPEGEncodeVideo final :public EncodeVideo
{
private:
    AVCodec *m_encodecodec = nullptr;
    AVCodecContext *m_codecContext= nullptr;
    AVFrame *m_avframe = nullptr;
    int m_frameindex = 0;
    int m_srcindex = 0;
    SwsContext *m_swscontext = nullptr ;
    std::function<void(int,AVPacket&,int)>  m_writefilecallback;
    bool m_initialized = false;
    std::mutex m_write_mutex;
public:
    FFMPEGEncodeVideo(/* args */);
    ~FFMPEGEncodeVideo();

    /** * @brief  初始化编码
     * @param   encoderinfo   视频参数信息
     * @param   srcindex   视频流编号
     * @param   writefilecallback   写入文件回调
     * @return  0: sucess ** **/
    int Initencoder(InputInfo& encoderinfo, int srcindex, std::function<void(int,AVPacket&,int)>  writefilecallback);

    /** * @brief  推入图片数据
     * @param   rgb   rgb图片数据
     * @param   size  图片大小
     * @param   srcpixfmt  图片格式
     * @return  0: sucess ** **/
    int WriteRGBData(const uint8_t *rgb,int size,int srcpixfmt);

    /** * @brief  结束编码
     * @param   
     * @return  ** **/  
    void EndEncode();
private:
    /** * @brief  打开视频文件
     * @param   opt_arg  图片格式
     * @return ** **/
    void OpenVideo(AVDictionary *opt_arg);

    /** * @brief  初始化SwsContext
    * @param   srcpixfmt  图片格式
    * @return ** **/
    int InitSwsContext(enum AVPixelFormat srcpixfmat);

    /** * @brief 编码
    * @param   frame  编码数据
    * @return ** **/
    void encode(const AVFrame* frame);

    /** * @brief 释放资源
        * @return ** **/
    void Release();

};


