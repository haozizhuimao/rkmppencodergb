#pragma once

#include <string>

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

enum eStreamType
{
    H264 = 0x00,
    H265 = 0x01,
    WMV = 0x03,
};

struct FrameInfo {
    uint32_t height;
    uint32_t width;
    short format;
    uint8_t fps;
    
};

struct StreamInfo {
    short StreamType;
    uint32_t gop;
};

typedef struct {
        uint8_t *data;
        uint32_t size;
} SpsHeader;