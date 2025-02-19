#include "../encode/encodemanage.h"
#include <iostream>
#include <map>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <string>
// #include "encodemanage.h"

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#else
#include <sys/types.h>
#include <dirent.h>
#endif

EncodeManage::EncodeManage(/* args */)
{
    m_muxingvideo = std::make_shared<MuxingVideo>();
    m_encode_flag.store(false);
    m_finish_flag.store(false);
    m_3dresultflag.store(false);
    m_encodevideo_thread = std::thread(&EncodeManage::encodevideoThread, this);
    m_3dreslutqueue.clear();
    m_encode_queue.clear();
   
}

EncodeManage::~EncodeManage() {
    Release();
}

bool EncodeManage::PutFrame(int srcindex,uint8_t* data, uint32_t size,int srcpixfmt)
{
    if (!m_encodeflag || m_encoderptrvec.size() < srcindex){
    //    XQTKLOG_LOGGER_ERROR(xqtklog::getptr(),"RKEncodeVideo not init!");
        return false;
    }
    
    if (nullptr !=  m_encoderptrvec[srcindex]){
       XQTKLOG_LOGGER_NOFILE_DEBUG(xqtklog::getptr(),"PutFrame %d",srcindex);
        m_encoderptrvec[srcindex]->WriteRGBData(data,size,srcpixfmt);
    }else{
       // XQTKLOG_LOGGER_ERROR(xqtklog::getptr(),"srcindex is error");
        return false;
    }
     
    return true; 
}

 void EncodeManage::EndEncode()
 {   
    if (nullptr == m_muxingvideo || !m_encodeflag ){
       return;
    }
    m_encodeflag = false;
   // m_pvideoencoder->Release();
   for (auto& it: m_encoderptrvec){
        it->EndEncode();
   }
   m_muxingvideo->finished();
   m_encoderptrvec.clear();

 }

 bool EncodeManage::Init(const char *filename,std::vector<InputInfo> inputinfo)
 {
    m_encoderptrvec.clear();
    int i = 0;
    for (auto& it:inputinfo) {
        auto package_callback = std::bind(&MuxingVideo::writeVideo, m_muxingvideo, std::placeholders::_1, std::placeholders::_2,std::placeholders::_3);
        if (0==it.rkencode){
            #ifdef RK
            std::shared_ptr<EncodeVideo> pvideoencoder = std::make_shared<RKEncodeVideo>();
            auto ret = pvideoencoder->Initencoder(it,i,package_callback);
            if (0 != ret){
                XQTKLOG_LOGGER_ERROR(xqtklog::getptr(),"RKEncodeVideo intiecndoer failed!");
                return false;
            }
            
            m_encoderptrvec.emplace_back(pvideoencoder);
            #else
            std::shared_ptr<EncodeVideo> pvideoencoder = std::make_shared<FFMPEGEncodeVideo>();
            auto ret =  pvideoencoder->Initencoder(it,i,package_callback);
            if (0 != ret){
                XQTKLOG_LOGGER_ERROR(xqtklog::getptr(),"FFMPEGEncodeVideo intiecndoer failed!");
                return false;
            }
            m_encoderptrvec.emplace_back(pvideoencoder);
            #endif
        } else {
          //  std::cout << "EncodeManage::init FFMPEGEncodeVideo \r\n";
            std::shared_ptr<EncodeVideo> pvideoencoder = std::make_shared<FFMPEGEncodeVideo>();
            auto ret = pvideoencoder->Initencoder(it,i,package_callback);
            if (0 != ret){
                XQTKLOG_LOGGER_ERROR(xqtklog::getptr(),"FFMPEGEncodeVideo intiecndoer failed!!!!!!");
                return false;
            }
            m_encoderptrvec.emplace_back(pvideoencoder);
        }
        i++;
        
    }
    auto ret =  m_muxingvideo->Init(filename,inputinfo);
    if (0 != ret){
        XQTKLOG_LOGGER_ERROR(xqtklog::getptr(),"MuxingVideo init failed:%d",ret);
    }   
     m_encodeflag = true;
   
    return true;
 }

 bool EncodeManage::Init(const char* outfilename, FrameInfo& frame_info, StreamInfo& stream_info)
 {
    InputInfo inputinfo;
    if (frame_info.fps <= 0){
            inputinfo.fps = 30;
    }else {
            inputinfo.fps = frame_info.fps;
    }
        
    inputinfo.height = frame_info.height;
    inputinfo.width = frame_info.width;
    inputinfo.size = 0;
    inputinfo.avcodeid = AV_CODEC_ID_H264;
    inputinfo.pixfmt = AV_PIX_FMT_YUV420P;
    inputinfo.format = frame_info.format;
    const char* codecname = "h264_nvenc";
    memcpy(inputinfo.codecname,codecname,strlen(codecname));
    #ifdef RK
      inputinfo.rkencode = 0;
    #else
        inputinfo.rkencode = 1;
    #endif
    std::vector<InputInfo> vecInputinfo;
    vecInputinfo.emplace_back(inputinfo);

    return Init(outfilename,vecInputinfo);
 }


void EncodeManage::StartRecordVideo(const char* outfilename,int fps,int videoWidth,int videoHeight,bool IsRecordDepth,bool detectresultflag)
{
    if(fps >30 || fps <= 0){
        fps = 30;
    }
    XQTKLOG_LOGGER_NOFILE_INFO(xqtklog::getptr(),"StartRecordVideo Release:%s,pfs:%d,width:%d,height:%d,depth:%d,detect:%d",outfilename,fps,videoWidth,videoHeight,IsRecordDepth,detectresultflag);
    m_3dreslutqueue.clear();
    m_encode_queue.clear();
    InputInfo info1;
    info1.width = videoWidth;
    info1.height = videoHeight;
    info1.fps = fps;
   // std::cout << "*********************** info1.fps :" <<  info1.fps  << ":" << fps << std::endl;
    info1.avcodeid = AV_CODEC_ID_H264;
    info1.pixfmt = AV_PIX_FMT_YUV420P;
    info1.size = 0;
    info1.rkencode = 0;
    info1.format = 1;
    const char* codecname = "h264_nvenc";
    memcpy(info1.codecname,codecname,strlen(codecname));
  //  info1.codecname = "h264_nvenc";
    std::vector<InputInfo> vecInputinfo;
    vecInputinfo.emplace_back(info1);
    m_3dresultflag.store(detectresultflag);

    m_fps = fps;
    m_videowidth = videoWidth;
    m_videoheight = videoHeight;
    m_IsRecordDepth = IsRecordDepth;
    if(IsRecordDepth){
        InputInfo info2;
        info2.width = 640;
        info2.height = 400;
        info2.fps = fps;
        info2.avcodeid = AV_CODEC_ID_RAWVIDEO;
        info2.pixfmt = AV_PIX_FMT_GRAY16BE;
        info2.size = 0;
        info2.rkencode = 1;
        // info2.codecname = "rawvideo";
        const char* codecname = "rawvideo";
        memcpy(info2.codecname,codecname,strlen(codecname));
        vecInputinfo.emplace_back(info2);
    }

   auto bret = Init(outfilename,vecInputinfo);
   if(bret){
         m_encode_flag.store(true);
        // m_3dresultflag.store(detectresultflag);
        m_cv_encode.notify_all();
        XQTKLOG_LOGGER_NOFILE_DEBUG(xqtklog::getptr(),"init sucess");
   }else{
        XQTKLOG_LOGGER_NOFILE_INFO(xqtklog::getptr(), "init error");
   }
}

void EncodeManage::StopRecord()
{
    m_encode_flag.store(false);
    m_3dresultflag.store(false);
    EndEncode(); 
    XQTKLOG_LOGGER_NOFILE_DEBUG(xqtklog::getptr(),"EndEncode sucess");
}

void EncodeManage::encodevideoThread()
{
    int fpsnum = 0;
    int count = 0;
    while (!m_finish_flag){
        if (!m_encode_flag.load()){
         //   XQTKLOG_LOGGER_NOFILE_INFO(xqtklog::getptr(), "encodevideoThread  wait");
            std::unique_lock<std::mutex> lock(m_encodemutex);
            m_cv_encode.wait(lock, [this]() {return m_encode_flag.load(); });
            if(m_finish_flag.load()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if(m_fps < 30){
                // if(0 == 30 % m_fps){
                    double num = 30.0 / m_fps;
                    fpsnum = static_cast<int>(num+0.5);
                    count = fpsnum;
                // }
            }
            continue;
        }
        auto data = m_encode_queue.pop();
        if (nullptr == data){
            break;
        }
        
        if(count < fpsnum && 0 != fpsnum ){
            count++;
            continue;
        }
        count = 1;
        if(m_3dresultflag.load()){
            if(!ImageHas3dresult(data->ts)){
                continue;
            }
        }
        if(!data->color_mat.empty()){
            if (m_videowidth != data->color_mat.cols ||  m_videoheight != data->color_mat.rows){
                cv::Mat tmp;
                cv::resize(data->color_mat,tmp,cv::Size(m_videowidth,m_videoheight));
                int size = tmp.cols * tmp.rows * tmp.channels();
                PutFrame(0,tmp.data,size,2);
                 XQTKLOG_LOGGER_NOFILE_DEBUG(xqtklog::getptr(),"push data");
            }else {
                int size = data->color_mat.cols * data->color_mat.rows * data->color_mat.channels();
                PutFrame(0,data->color_mat.data,size,2);
                 XQTKLOG_LOGGER_NOFILE_DEBUG(xqtklog::getptr(),"push data 1");
            }    
        }
        if(m_IsRecordDepth){
            if(!data->depth_mat.empty()){
                cv::Mat dst = cv::Mat::zeros(data->depth_mat.size(), CV_8UC4);
                data->depth_mat.convertTo(dst, CV_16U, 1.0, 0);
                int size = dst.cols * dst.rows * dst.channels();
                PutFrame(1,dst.data,size,29);            
            }
        }

    }
    XQTKLOG_LOGGER_NOFILE_INFO(xqtklog::getptr(), "EncodevideoThread End !!!");
}
void EncodeManage::Release()
{
    m_finish_flag.store(true);
    m_encode_queue.clear();
    m_encode_queue.push(nullptr);
    m_encode_flag.store(true);
    m_cv_encode.notify_all();
    if (m_3dreslutqueue.isFull()){
        m_3dreslutqueue.pop();
    }
    m_3dreslutqueue.push(-2);
    if(m_encodevideo_thread.joinable()){
        m_encodevideo_thread.join();
    }
    XQTKLOG_LOGGER_NOFILE_INFO(xqtklog::getptr(),"FFMPEGEncodeVideo Release");
}

void EncodeManage::PushImageData(std::shared_ptr<ColorAndDepthMat>& imagedata)
{
    if(m_encode_flag.load()){
        if (m_encode_queue.isFull()){
            m_encode_queue.pop();
        }
        m_encode_queue.push(imagedata);
        if(nullptr != imagedata){
            XQTKLOG_LOGGER_NOFILE_DEBUG(xqtklog::getptr(),"image ts:%lld",imagedata->ts);
        }  
    }
}

void EncodeManage::Push3DResultData(long long ts)
{
    if(m_3dresultflag.load() && m_encode_flag.load()){
        if (m_3dreslutqueue.isFull()){
            m_3dreslutqueue.pop();
        }
        m_3dreslutqueue.push(ts);
        XQTKLOG_LOGGER_NOFILE_DEBUG(xqtklog::getptr(),"3d Result ts:%lld",ts);
    }
}

 bool EncodeManage::ImageHas3dresult(long long ts)
 {
    static long long rests = -1;
    if(!m_encode_flag || m_finish_flag) {
        return false;
    }
    if(rests < 0){
         rests = m_3dreslutqueue.pop();
         if(rests < -1) {
            return false;
         }
    }
    if(!m_encode_flag || m_finish_flag) {
        return false;
    }
    XQTKLOG_LOGGER_NOFILE_DEBUG(xqtklog::getptr(),"3d Result ts:%lld,image ts:%lld",rests, ts);
    if(ts == rests){
        rests = -1;
        return true;
    } else if(ts < rests){
        return false;
    } 
    rests = -1;
    return  ImageHas3dresult(ts);
    
 }