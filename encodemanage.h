#pragma once

#ifdef RK
    #include "rk/encodevideork.h"
#endif
#include "muxingvideo.h"
#include "encodevideoffmpeg.h"
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "producer_consumer_queue.h"
#include "pose_detect_common.h"

class EncodeManage
{
private:

    std::vector<std::shared_ptr<EncodeVideo>> m_encoderptrvec;   //视频编码对象
    std::shared_ptr<MuxingVideo> m_muxingvideo = nullptr;       //视频存储指针
    bool m_encodeflag = false;                                    //是否编码标志
    int m_videowidth = 0;                                       //视频宽
    int m_videoheight = 0;                                      //视频高

    ProducerConsumerQueue<std::shared_ptr<ColorAndDepthMat>> m_encode_queue{100};   // 需要编码的图片数据

    ProducerConsumerQueue<long long> m_3dreslutqueue{100};              // 3d结果数据时间戳
    
    std::thread m_encodevideo_thread;           //编码线程
    std::atomic_bool m_encode_flag = false;     //停止标志
    std::atomic_bool m_finish_flag = false;     //结束标志
    std::mutex m_encodemutex;                   //条件变量
    std::condition_variable m_cv_encode;        //开始采集数据
    bool m_IsRecordDepth = false;               //是否编码深度图
    int m_fps = 30;                             //视频帧率   
    std::atomic_bool m_3dresultflag = false;    //是否只编码有3d结果的图片 
public:

private:
     /** * @brief 视频编码线程
     * @param  
     * @return   ** **/
    void encodevideoThread();

     /** * @brief 判断图片是否有3d数据结果
     * @param  ts 需要判断图片数据戳
     * @return   ** **/
    bool ImageHas3dresult(long long ts);
public:
    EncodeManage(/* args */);
    ~EncodeManage();

    /** * @brief  推入图片数据
     * @param   data   图片数据
     * @param   size   数据大小
     * @return  0: sucess ** **/
    bool PutFrame(int srcindex,uint8_t* data, uint32_t size,int srcpixfmt);

     /** * @brief 结束编码
     * @param     
     * @return  ** **/
    void EndEncode();

    /** * @brief 编码指定路径下的图片
     * @param   dirpath   文件路径
     * @param   videoFilePath   frame信息
     * @param   fps   fps
     * @param   videoWidth   宽
     * @param   videoHeight   高
     * @return  0: sucess ** **/
    bool MakeVideo(char* dirpath,char* videoFilePath,int fps,int videoWidth,int videoHeight){ return false;}

     /** * @brief 初始化视频编码
     * @param filename 视频文件名
     * @param inputinfo 编码视频的参数信息
     * @return   ** **/
    bool Init(const char *filename,std::vector<InputInfo> inputinfo);  

     /** * @brief 初始化视频编码
     * @param outfilename 视频文件名
     * @param frame_info 编码视频的参数信息
     * @param stream_info 编码视频流参数信息
     * @return   ** **/
    bool Init(const char* outfilename, FrameInfo& frame_info, StreamInfo& stream_info);

      /** * @brief 编码视频
     * @param   outfilename   视频文件名
     * @param   fps   fps
     * @param   videoWidth   宽
     * @param   videoHeight   高
     * @param   IsRecordDepth   编码深度图标志
     * @param   detectresultflag   只编码有数据帧标志
     * @return   ** **/
    void StartRecordVideo(const char* outfilename,int fps,int videoWidth,int videoHeight,bool IsRecordDepth,bool detectresultflag );

    /** * @brief 停止视频编码
     * @param  
     * @return   ** **/
    void StopRecord();

    /** * @brief 释放资源
     * @param  
     * @return   ** **/
    void Release();

    /** * @brief 推入需要编码的图片
     * @param  imagedata 图片数据
     * @return   ** **/
    void PushImageData(std::shared_ptr<ColorAndDepthMat>& imagedata);

    /** * @brief 推入有3d结果的数据图片时间戳
     * @param   时间戳
     * @return   ** **/
    void Push3DResultData(long long ts);
};

