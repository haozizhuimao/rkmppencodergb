#pragma once 
#include "../encode/commoninfo.h"
#include <functional>
#include "../../logger/include/xqtklog.h"
extern "C"
{
	#include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
};


class EncodeVideo
{
private:
    /* data */
public:
    EncodeVideo(/* args */) = default;
    virtual ~EncodeVideo() = default;

    /** * @brief  初始化编码
     * @param   encoderinfo   视频参数信息
     * @param   srcindex   视频流编号
     * @param   writefilecallback   写入文件回调
     * @return  0: sucess ** **/
    virtual int Initencoder(InputInfo& encoderinfo, int srcindex, std::function<void(int,AVPacket&,int)>  writefilecallback) = 0;

    /** * @brief  推入图片数据
     * @param   rgb   rgb图片数据
     * @param   size  图片大小
     * @param   srcpixfmt  图片格式
     * @return  0: sucess ** **/
    virtual int WriteRGBData(const uint8_t *rgb,int size,int srcpixfmt) = 0;

    /** * @brief  结束编码
     * @param   
     * @return  ** **/  
    virtual void EndEncode() = 0;
};

