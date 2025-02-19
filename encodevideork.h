#pragma once

#include <rockchip/rk_mpi.h>
#include "../encode/encodevideo.h"
#include <functional>
#include <string>
#include <thread>
// #include "../encode/commoninfo.h"



//编码格式
struct MppEncInfo {
    int32_t width;
    int32_t height;
    int32_t hor_stride;
    int32_t ver_stride;
    int32_t frame_size;
    int32_t header_size;
    int32_t mdinfo_size;
    int32_t bps;
    MppCodingType code_type;
    MppFrameFormat frame_format;

    MppEncRcMode rc_mode;
    MppEncSeiMode sei_mode;
    MppEncHeaderMode header_mode;
};


class RKEncodeVideo final:public EncodeVideo
{
public:
    RKEncodeVideo();
    ~RKEncodeVideo(void);

    /** * @brief  初始化编码
     * @param   encoderinfo   视频参数信息
     * @param   srcindex   视频流编号
     * @param   writefilecallback   写入文件回调
     * @return  0: sucess ** **/
    virtual int Initencoder(InputInfo& encoderinfo, int srcindex, std::function<void(int,AVPacket&,int)>  writefilecallback);

    /** * @brief  推入图片数据
     * @param   rgb   rgb图片数据
     * @param   size  图片大小
     * @param   srcpixfmt  图片格式
     * @return  0: sucess ** **/
    virtual int WriteRGBData(const uint8_t *rgb,int size,int srcpixfmt);

      /** * @brief  结束编码
     * @param   
     * @return  ** **/  
    virtual void EndEncode();
private:
     /** * @brief  接收编码结果线程
     * @param   
     * @return  0:  ** **/
    void EncRecvThread();

    /** * @brief  设置mpp 编码资源
     * @return ** **/
    bool SetMppEncCfg(void);

    /** * @brief  初始化Mpp资源
     * @return ** **/
    void InitMppEnc();

    /** * @brief  申请Drm内存
     * @return ** **/
    bool AllocterDrmbuf();

    /** * @brief  初始化MPP API
     * @return ** **/
    bool InitMppAPI();

    /** * @brief  获取视频流head信息
     * @param   sps_header   视频head信息
     * @return ** **/
    void GetHeaderInfo(SpsHeader *sps_header);

    /** * @brief  转换视频流信息为mpp视频格式
     * @param   stream_type   视频流类型
     * @return mpp视频流格式 ** **/
    MppCodingType AdaptStreamType(short stream_type);

    /** * @brief  转换frame格式为mpp格式
     * @param   stream_type   frame 类型
     * @return mppframe格式 ** **/
    MppFrameFormat AdaptFrameType(short stream_type);

    /** * @brief  获取frame大小
     * @param   frame_format   frame 类型
     * @param   hor_stride  
     * @param   ver_stride  
     * @return  ** **/
    int GetFrameSize(MppFrameFormat frame_format, int32_t hor_stride, int32_t ver_stride);

     /** * @brief  获取视频head信息大小
     * @param   frame_format   frame 类型
     * @param   width  
     * @param   height  
     * @return  ** **/
    int GetHeaderSize(MppFrameFormat frame_format, uint32_t width, uint32_t height);

    /** * @brief  释放资源
     * @return ** **/
    void Release();

      /** * @brief  封装为其他格式
     * @param   data   编码后的数据
     * @param   size   数据大小 
     * @return  ** **/
    void Packaging(uint8_t* data,uint32_t size);

private:
    MppCtx m_mppctx = nullptr;
    MppApi* m_mppapi = nullptr;
    MppEncCfg m_mppcfg = nullptr;
    MppBufferGroup m_grpbuffer = nullptr;
    MppBuffer m_frame_buf = nullptr;
    MppBuffer m_pkt_buf = nullptr;
    MppFrame m_frame = nullptr;

    FrameInfo m_frame_info;     //frame 信息
    StreamInfo m_stream_info;   //视频流信息
    MppEncInfo m_enc_info;      //编码格式数据

    bool m_is_running = false;  //是否编码
    std::thread m_recv_thread;  //接收编码结果线程

    std::function<void(int,AVPacket&,int)>  m_writefilecallback;

    int m_put_num = 0;              //接收到的编码帧数量
    int m_encode_num = 0;           //完成编码帧数量
    int m_srcindex = 0;            //视频流编号
    int m_frame_index=0;        //帧序号
};