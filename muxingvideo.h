#pragma once
#include "commoninfo.h"
#include "../../logger/include/xqtklog.h"

extern "C"
{
	#include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
};
 
 #include <vector>
 #include <memory>
 #include <mutex>


typedef struct OutputStream {
    AVCodec *encodecodec = nullptr;
    AVCodecContext *codecContext= nullptr;
    AVStream *stream = nullptr;
    uint8_t extradata[256]; 
    int rkflag = 0;
} OutputStream;



class MuxingVideo
{
private:
    
    AVFormatContext * m_outformt_ctx = nullptr;
    bool m_initialized = false;
    std::vector<std::shared_ptr<OutputStream >> m_outputstream; 
    std::mutex m_mtx;
    int m_fps = 30;             //fps
    
public:
    MuxingVideo() = default;
    ~MuxingVideo();

    /** * @brief 结束存储
     * @param     
     * @return  ** **/
    void finished();

    /** * @brief 初始化编码信息
     * @param   filename   编码后文件名
     * @param   frame_info   frame信息
     * @param   stream_info   视频流信息
     * @return  0: sucess ** **/
    int Init(const char *filename,std::vector<InputInfo> inputinfo);
    
    
      /** * @brief  推入图片数据
     * @param   rgb   rgb图片数据
     * @return  0: sucess ** **/
    void writeVideo(int no,AVPacket& pkt,int rkflag);
private:

    /** * @brief 写入视频头部信息
         * @return ** **/
    int WriteHeader();

     /** * @brief 释放资源
         * @return ** **/
    void Release();

    /** * @brief rk添加视频流信息
     * @param   optstream   视频流信息
     * @param   InputInfo   视频输入参数
     * @return   ** **/
    void RKAddStream(std::shared_ptr<OutputStream> optstream,InputInfo& InputInfo);

    /** * @brief ffmpeg添加视频流信息
     * @param   optstream   视频流信息
     * @param   InputInfo   视频输入参数
     * @return   ** **/
    void AddStream(std::shared_ptr<OutputStream> optstream,InputInfo& InputInfo);

};

