#pragma once

#include <rockchip/rk_mpi.h>
#include <functional>
#include <string>
#include <thread>
#include "commoninfo.h"

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
    // MppBufferGroup buf_grp;
    // MppBuffer frm_buf;
    // MppBuffer pkt_buf;
    // MppBuffer md_info;
    MppEncSeiMode sei_mode;
    MppEncHeaderMode header_mode;
};


class VideoEncoder {
public:
    VideoEncoder();
    ~VideoEncoder(void);

    bool Init(const FrameInfo& frame_info, const StreamInfo& stream_info,const std::function<void(uint8_t* data, uint32_t size)>& package_callback,SpsHeader *sps_header);

    bool PutFrame(uint8_t* data, uint32_t size);

    void Release();

private:
    static void EncRecvThread(VideoEncoder* self);
    bool SetMppEncCfg(void);
    void InitMppEnc();
    bool AllocterDrmbuf();
    bool InitMppAPI();
    void GetHeaderInfo(SpsHeader *sps_header);
    MppCodingType AdaptStreamType(short stream_type);
    MppFrameFormat AdaptFrameType(short stream_type);
    int GetFrameSize(MppFrameFormat frame_format, int32_t hor_stride, int32_t ver_stride);
    int GetHeaderSize(MppFrameFormat frame_format, uint32_t width, uint32_t height);



private:
    MppCtx m_mppctx = nullptr;
    MppApi* m_mppapi = nullptr;
    MppEncCfg m_mppcfg = nullptr;
    // MppEncRcCfg rc_cfg_;
    // MppEncPrepCfg prep_cfg_;
    // MppEncCodecCfg codec_cfg_;

    // int m_timeout = -1;
    FrameInfo m_frame_info;
    StreamInfo m_stream_info;
    MppEncInfo m_enc_info;

    bool m_is_running = false;
    std::thread m_recv_thread;
    std::function<void(uint8_t* data, uint32_t size)> package_callback_;

    MppBufferGroup m_grpbuffer = nullptr;
    MppBuffer m_frame_buf = nullptr;
    MppBuffer m_pkt_buf = nullptr;
    MppFrame m_frame = nullptr;
    int m_put_num = 0;
    int m_encode_num = 0;
};