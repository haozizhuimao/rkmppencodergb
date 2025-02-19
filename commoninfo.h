#pragma once

#include <string>
#include <cstdint>

//视频head信息
typedef struct {
        uint8_t data[256];      //head数据
        uint32_t size;      //数据大小
} SpsHeader;


// 输入图片的格式
enum  eFormatType
{
    BGR24 = 0x00,
    RGB24 = 0x01,
    ARGB32 = 0x02,
    ABGR32 = 0x03,
    BGRA32 = 0x04,
    RGBA32 = 0x05,
    // YUV420SP = 0x06,
    // YUV420P = 0x07,
};


//编码结果流格式
enum eStreamType
{
    H264 = 0x00,
    H265 = 0x01,
    WMV = 0x03,
};

struct FrameInfo {
    uint32_t height;    //frame高
    uint32_t width;     //frame宽
    short format;   //eFormatType
    uint32_t fps;    //帧率
    
};

struct StreamInfo {
    short StreamType;   //eStreamType
    uint32_t gop;       //
};

typedef struct InputInfo{
    int width = 0;          //视频宽
    int height = 0;         //视频高
    int fps = 30;           //视频帧率
    short avcodeid = 27;    //编码器id
    short pixfmt = 0;       //编码的数据格式
    char codecname[256];    //编码器名称
    uint8_t data[256];      //head数据
    uint32_t size;          //数据大小
    int rkencode = 0;       //是否使用rkmpp编码
    short format;           //eFormatType
} InputInfo ;