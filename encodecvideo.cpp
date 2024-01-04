#include "encodecvideo.h"
#include <cstring>
#include <iostream>
// #include <spdlog/spdlog.h>
// #include <magic_enum.hpp>

#define MPP_ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#define SZ_1K (1024)
#define SZ_2K (SZ_1K * 2)
#define SZ_4K (SZ_1K * 4)


VideoEncoder::VideoEncoder()
    :m_is_running(true)
{
}

VideoEncoder::~VideoEncoder(void)
{
    Release();
}

void VideoEncoder::Release()
{
     m_is_running = false;
    if(m_recv_thread.joinable())
    {
        m_recv_thread.join();
    }
    if (m_mppctx != nullptr)
    {
        mpp_destroy(m_mppctx);
    }
    if (nullptr != m_frame_buf) 
    {
        mpp_buffer_put(m_frame_buf);
        m_frame_buf = nullptr;
    }
    
    if (nullptr !=  m_pkt_buf) 
    {
        mpp_buffer_put(m_pkt_buf);
        m_pkt_buf = nullptr;
    }

    if (nullptr != m_grpbuffer ) {
        mpp_buffer_group_put(m_grpbuffer);
       m_grpbuffer = nullptr;
    }
}
MppCodingType VideoEncoder::AdaptStreamType(short stream_type)
{
    switch (stream_type)
    {
    case eStreamType::H264 :
        return MPP_VIDEO_CodingAVC;
    case eStreamType::H265 :
        return MPP_VIDEO_CodingHEVC;
    case eStreamType::WMV :
        return MPP_VIDEO_CodingWMV;
    default:
        break;
    }
    return MPP_VIDEO_CodingUnused;
}

MppFrameFormat VideoEncoder::AdaptFrameType(short stream_type)
{
    switch (stream_type)
    {
    case eFormatType::BGR24:
        return MPP_FMT_BGR888;
    case eFormatType::RGB24:
        return MPP_FMT_RGB888;
    case eFormatType::ARGB32:
        return MPP_FMT_ARGB8888;
    case eFormatType::ABGR32:
        return MPP_FMT_ABGR8888;
    case eFormatType::BGRA32:
        return MPP_FMT_BGRA8888;
    case eFormatType::RGBA32:
        return MPP_FMT_RGBA8888;
    // case eFormatType::YUV420SP:
    //     return MPP_FMT_YUV420SP;
    // case eFormatType::YUV420P:
    //     return MPP_FMT_YUV420P;
    default:
        break;
    }
    return MPP_FMT_BUTT;
}

int VideoEncoder::GetFrameSize(MppFrameFormat frame_format, int32_t hor_stride, int32_t ver_stride)
{
    int32_t frame_size = 0;
    switch (frame_format & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP:
    case MPP_FMT_YUV420P: {
        m_enc_info.hor_stride = MPP_ALIGN(hor_stride , 16);
        frame_size = MPP_ALIGN( m_enc_info.hor_stride, 16) * MPP_ALIGN(ver_stride, 16) * 3 / 2;
    } break;

    case MPP_FMT_YUV422_YUYV:
    case MPP_FMT_YUV422_YVYU:
    case MPP_FMT_YUV422_UYVY:
    case MPP_FMT_YUV422_VYUY:
    case MPP_FMT_YUV422P:
    case MPP_FMT_YUV422SP: {
        m_enc_info.hor_stride = MPP_ALIGN(hor_stride , 16);
        frame_size = MPP_ALIGN(m_enc_info.hor_stride, 16) * MPP_ALIGN(ver_stride, 16) * 2;
    } break;
    // case MPP_FMT_RGB444:
    // case MPP_FMT_BGR444:
    // case MPP_FMT_RGB555:
    // case MPP_FMT_BGR555:
    // case MPP_FMT_RGB565:
    // case MPP_FMT_BGR565:
    case MPP_FMT_RGB888:
    case MPP_FMT_BGR888:
    // case MPP_FMT_RGB101010:
    // case MPP_FMT_BGR101010:
    {
        m_enc_info.hor_stride = MPP_ALIGN(hor_stride *3 , 16);
        frame_size = MPP_ALIGN(m_enc_info.hor_stride, 16) * MPP_ALIGN(ver_stride, 16);
    }
    break;
    case MPP_FMT_ARGB8888:
    case MPP_FMT_ABGR8888:
    case MPP_FMT_BGRA8888:
    case MPP_FMT_RGBA8888: 
    {
        m_enc_info.hor_stride = MPP_ALIGN(hor_stride *4 , 16);
        frame_size = MPP_ALIGN(m_enc_info.hor_stride, 16) * MPP_ALIGN(ver_stride, 16);
    } break;

    default: {
        frame_size = MPP_ALIGN(hor_stride, 16) * MPP_ALIGN(ver_stride, 16) * 4;
    } break;
    }
    return frame_size;
}

int VideoEncoder::GetHeaderSize(MppFrameFormat frame_format, uint32_t width, uint32_t height)
{
    int header_size = 0;
    if (MPP_FRAME_FMT_IS_FBC(frame_format)) {
        if ((frame_format & MPP_FRAME_FBC_MASK) == MPP_FRAME_FBC_AFBC_V1)
            header_size = MPP_ALIGN(MPP_ALIGN(width, 16) * MPP_ALIGN(height, 16) / 16, SZ_4K);
        else
            header_size = MPP_ALIGN(width, 16) * MPP_ALIGN(height, 16) / 16;
    } else {
        header_size = 0;
    }
    return header_size;
}

bool VideoEncoder::SetMppEncCfg(void)
{
    mpp_enc_cfg_set_s32(m_mppcfg, "prep:width", m_enc_info.width);
    mpp_enc_cfg_set_s32(m_mppcfg, "prep:height", m_enc_info.height);
    mpp_enc_cfg_set_s32(m_mppcfg, "prep:hor_stride", m_enc_info.hor_stride);
    mpp_enc_cfg_set_s32(m_mppcfg, "prep:ver_stride", m_enc_info.ver_stride);
    mpp_enc_cfg_set_s32(m_mppcfg, "prep:format", m_enc_info.frame_format);

    mpp_enc_cfg_set_s32(m_mppcfg, "rc:mode", m_enc_info.rc_mode);

    /* fix input / output m_frame rate */
    mpp_enc_cfg_set_s32(m_mppcfg, "rc:fps_in_flex", 0);
    mpp_enc_cfg_set_s32(m_mppcfg, "rc:fps_in_num", m_frame_info.fps);
    mpp_enc_cfg_set_s32(m_mppcfg, "rc:fps_in_denorm", 1);
    mpp_enc_cfg_set_s32(m_mppcfg, "rc:fps_out_flex", 0);
    mpp_enc_cfg_set_s32(m_mppcfg, "rc:fps_out_num", m_frame_info.fps);
    mpp_enc_cfg_set_s32(m_mppcfg, "rc:fps_out_denorm", 1);
    mpp_enc_cfg_set_s32(m_mppcfg, "rc:gop", m_stream_info.gop ? m_stream_info.gop : m_frame_info.fps * 2);

    /* drop m_frame or not when bitrate overflow */
    mpp_enc_cfg_set_u32(m_mppcfg, "rc:drop_mode", MPP_ENC_RC_DROP_FRM_DISABLED);
    mpp_enc_cfg_set_u32(m_mppcfg, "rc:drop_thd", 20); /* 20% of max bps */
    mpp_enc_cfg_set_u32(m_mppcfg, "rc:drop_gap", 1); /* Do not continuous drop m_frame */

    /* setup bitrate for different rc_mode */
    mpp_enc_cfg_set_s32(m_mppcfg, "rc:bps_target", m_enc_info.bps);
    switch (m_enc_info.rc_mode) {
    case MPP_ENC_RC_MODE_FIXQP: {
        /* do not setup bitrate on FIXQP mode */
    } break;
    case MPP_ENC_RC_MODE_CBR: {
        /* CBR mode has narrow bound */
        mpp_enc_cfg_set_s32(m_mppcfg, "rc:bps_max", m_enc_info.bps * 17 / 16);
        mpp_enc_cfg_set_s32(m_mppcfg, "rc:bps_min", m_enc_info.bps * 15 / 16);
    } break;
    case MPP_ENC_RC_MODE_VBR:
    case MPP_ENC_RC_MODE_AVBR: {
        /* VBR mode has wide bound */
        mpp_enc_cfg_set_s32(m_mppcfg, "rc:bps_max", m_enc_info.bps * 17 / 16);
        mpp_enc_cfg_set_s32(m_mppcfg, "rc:bps_min", m_enc_info.bps * 1 / 16);
    } break;
    default: {
        /* default use CBR mode */
        mpp_enc_cfg_set_s32(m_mppcfg, "rc:bps_max", m_enc_info.bps * 17 / 16);
        mpp_enc_cfg_set_s32(m_mppcfg, "rc:bps_min", m_enc_info.bps * 15 / 16);
    } break;
    }

    /* setup qp for different codec and rc_mode */
    switch (m_enc_info.code_type) {
    case MPP_VIDEO_CodingAVC:
    case MPP_VIDEO_CodingHEVC: {
        switch (m_enc_info.rc_mode) {
        case MPP_ENC_RC_MODE_FIXQP: {
            RK_S32 fix_qp = 0;
            mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_init", fix_qp);
            mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_max", fix_qp);
            mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_min", fix_qp);
            mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_max_i", fix_qp);
            mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_min_i", fix_qp);
            mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_ip", 0);
        } break;
        case MPP_ENC_RC_MODE_CBR:
        case MPP_ENC_RC_MODE_VBR:
        case MPP_ENC_RC_MODE_AVBR: {
            mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_init", -1);
            mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_max", 51);
            mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_min", 10);
            mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_max_i", 51);
            mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_min_i", 10);
            mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_ip", 2);
        } break;
        default: {
        //    spdlog::error("unsupport encoder rc mode {}", m_enc_info.rc_mode);
        } break;
        }
    } break;
    case MPP_VIDEO_CodingVP8: {
        /* vp8 only setup base qp range */
        mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_init", 40);
        mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_max", 127);
        mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_min", 0);
        mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_max_i", 127);
        mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_min_i", 0);
        mpp_enc_cfg_set_s32(m_mppcfg, "rc:qp_ip", 6);
    } break;
    case MPP_VIDEO_CodingMJPEG: {
        /* jpeg use special codec config to control qtable */
        mpp_enc_cfg_set_s32(m_mppcfg, "jpeg:q_factor", 80);
        mpp_enc_cfg_set_s32(m_mppcfg, "jpeg:qf_max", 99);
        mpp_enc_cfg_set_s32(m_mppcfg, "jpeg:qf_min", 1);
    } break;
    default: {
    } break;
    }

    /* setup codec  */
    mpp_enc_cfg_set_s32(m_mppcfg, "codec:type", m_enc_info.code_type);
    switch (m_enc_info.code_type) {
    case MPP_VIDEO_CodingAVC: {
        RK_U32 constraint_set;
        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
        mpp_enc_cfg_set_s32(m_mppcfg, "h264:profile", 100);
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
        mpp_enc_cfg_set_s32(m_mppcfg, "h264:level", 40);
        mpp_enc_cfg_set_s32(m_mppcfg, "h264:cabac_en", 1);
        mpp_enc_cfg_set_s32(m_mppcfg, "h264:cabac_idc", 0);
        mpp_enc_cfg_set_s32(m_mppcfg, "h264:trans8x8", 1);

    } break;
    case MPP_VIDEO_CodingHEVC:
    case MPP_VIDEO_CodingMJPEG:
    case MPP_VIDEO_CodingVP8: {
    } break;
    default: {
   
    } break;
    }


    auto ret = m_mppapi->control(m_mppctx, MPP_ENC_SET_CFG, m_mppcfg);
    if (ret) 
    {
        return false;
    }
    return true;
}

void VideoEncoder::InitMppEnc()
{
    m_enc_info.width = m_frame_info.width;
    m_enc_info.height = m_frame_info.height;
    m_enc_info.code_type = AdaptStreamType(m_stream_info.StreamType);
    m_enc_info.frame_format = AdaptFrameType(m_frame_info.format);
    m_enc_info.ver_stride = MPP_ALIGN(m_frame_info.height, 16);
    m_enc_info.frame_size = GetFrameSize(m_enc_info.frame_format, m_frame_info.width, m_enc_info.ver_stride);
    m_enc_info.header_size = GetHeaderSize(m_enc_info.frame_format, m_frame_info.width, m_frame_info.height);
    m_enc_info.mdinfo_size = (MPP_VIDEO_CodingHEVC == m_enc_info.code_type) ? (MPP_ALIGN(m_enc_info.hor_stride, 32) >> 5) * (MPP_ALIGN(m_enc_info.ver_stride, 32) >> 5) * 16 : (MPP_ALIGN(m_enc_info.hor_stride, 64) >> 6) * (MPP_ALIGN(m_enc_info.ver_stride, 16) >> 4) * 16;
    m_enc_info.bps = m_frame_info.width * m_frame_info.height / 8 * m_frame_info.fps;
    m_enc_info.rc_mode = MPP_ENC_RC_MODE_VBR;
}

bool  VideoEncoder::AllocterDrmbuf()
{
    auto  ret = mpp_buffer_group_get_internal(&m_grpbuffer, MPP_BUFFER_TYPE_DRM);
    if (ret) {
        // spdlog::error("failed to get mpp buffer group ret {}", ret);
        return false;
    }

    ret = mpp_buffer_get(m_grpbuffer, &m_frame_buf, m_enc_info.frame_size + m_enc_info.header_size);
    if (ret) {
        // spdlog::error("failed to get buffer for input m_frame ret {}", ret);
        return false;
    }

    ret = mpp_buffer_get(m_grpbuffer, &m_pkt_buf, m_enc_info.frame_size);
    if (ret) {
        // spdlog::error("failed to get buffer for output packet ret {}", ret);
        return false;
    }

    return true;
}

bool VideoEncoder::InitMppAPI()
{ 
    auto ret = mpp_check_support_format(MPP_CTX_ENC, m_enc_info.code_type);
    if (ret != MPP_SUCCESS)
    {
        return false;
    }

    ret = mpp_create(&m_mppctx, &m_mppapi);
    if (ret != MPP_SUCCESS) {
        return false;
    }

    MppPollType timeout = MPP_POLL_NON_BLOCK;
    ret = m_mppapi->control(m_mppctx, MPP_SET_INPUT_TIMEOUT, &timeout);
    if (ret != MPP_SUCCESS) 
    {
        return false;
    }

    timeout = MPP_POLL_BLOCK;
    ret = m_mppapi->control(m_mppctx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
    if (ret != MPP_SUCCESS) 
    {
        return false;
    }

    ret = mpp_init(m_mppctx, MPP_CTX_ENC, m_enc_info.code_type);
    if (ret != MPP_SUCCESS) 
    {
        return false;
    }

    ret = mpp_enc_cfg_init(&m_mppcfg);
    if (ret != MPP_SUCCESS) 
    {
        return false;
    }

    ret = m_mppapi->control(m_mppctx, MPP_ENC_GET_CFG, m_mppcfg);
    if (ret) 
    {
        return false;
    }

    return true;
}

void VideoEncoder::GetHeaderInfo(SpsHeader *sps_header)
{
    MppPacket packet = NULL;
    auto ret = m_mppapi->control(m_mppctx, MPP_ENC_GET_EXTRA_INFO, &packet);
    if (ret)
    {
        printf("mpi control enc get extra info failed\n");
        return;
    }

    /* get and write sps/pps for H.264 */
    if (packet)
    {
        void *ptr   = mpp_packet_get_pos(packet);
        size_t len  = mpp_packet_get_length(packet);
        sps_header->data = (uint8_t*)malloc(len);
        sps_header->size = len;
        memcpy(sps_header->data,ptr,len);
        packet = NULL;
    }
}

bool VideoEncoder::Init(const FrameInfo& frame_info, const StreamInfo& stream_info,const std::function<void(uint8_t* data, uint32_t size)>& package_callback,SpsHeader *sps_header)
{
    m_frame_info = frame_info;
    m_stream_info = stream_info;
    m_put_num = 0;
    m_encode_num = 0;
    InitMppEnc();

    if (!AllocterDrmbuf())
    {
       return false;
    }
    if (!InitMppAPI())
    {
       return false;
    }
    if(!SetMppEncCfg()) 
    {
        return false;
    }

    GetHeaderInfo(sps_header);

    package_callback_ = package_callback;
    m_is_running = true;
    m_recv_thread = std::thread(&VideoEncoder::EncRecvThread, this);
    return true;
}

void VideoEncoder::EncRecvThread(VideoEncoder* self)
{
    MppPacket packet = NULL;
    RK_U32 eoi = 1;

    mpp_packet_init_with_buffer(&packet, self->m_pkt_buf);
    /* NOTE: It is important to clear output packet length!! */
    mpp_packet_set_length(packet, 0);

    while (self->m_is_running || self->m_encode_num != self->m_put_num)
    {
        auto ret = self->m_mppapi->encode_get_packet(self->m_mppctx, &packet);
        if (ret || NULL == packet) {
        //    spdlog::error("Get Package error, {}", ret);
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
            continue;
        }
        self->m_encode_num++;
        auto data = (uint8_t*)mpp_packet_get_pos(packet);
        auto len = mpp_packet_get_length(packet);
  
        auto pkt_eos = mpp_packet_get_eos(packet);
        /* for low delay partition encoding */
        if (mpp_packet_is_partition(packet))
        {
            eoi = mpp_packet_is_eoi(packet);
        }

        self->package_callback_(data, len);

        ret = mpp_packet_deinit(&packet);
        // assert(ret == MPP_SUCCESS);
    }
}

bool VideoEncoder::PutFrame(uint8_t* data, uint32_t size)
{   
    if (nullptr == data)
    {
        return false;
    }
    void *buf = mpp_buffer_get_ptr(m_frame_buf);
    if (nullptr == buf )
    {
         return false;
    }

    memcpy(buf,data,size);
    auto ret = mpp_frame_init(&m_frame);
    if (ret) 
    {
        return false;
    }

    mpp_frame_set_width(m_frame, m_enc_info.width);
    mpp_frame_set_height(m_frame, m_enc_info.height);
    mpp_frame_set_hor_stride(m_frame, m_enc_info.hor_stride);
    mpp_frame_set_ver_stride(m_frame, m_enc_info.ver_stride);
    mpp_frame_set_fmt(m_frame, m_enc_info.frame_format);
    mpp_frame_set_eos(m_frame, 0);

    mpp_frame_set_buffer(m_frame, m_frame_buf);

    ret = m_mppapi->encode_put_frame(m_mppctx, m_frame);
    if (ret != MPP_SUCCESS) 
    {
      mpp_frame_deinit(&m_frame);
        return false;
    }
    m_put_num++;
    mpp_frame_deinit(&m_frame);
    return true;
}
