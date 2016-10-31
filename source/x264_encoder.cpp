#include "x264_encoder.h"
#include "log.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "video_header.h"
x264_encoder::x264_encoder():
m_luma_size(0),
m_chroma_size(0),
m_fd(NULL)
{
}
x264_encoder::~x264_encoder()
{

}
bool x264_encoder::Open(const x264_encoder_t param)
{   
    /* Get default params for preset/tuning */
    if( x264_param_default_preset( &m_x264_param, "medium", NULL ) < 0 )
    {
        log_printf(LOG_LEVEL_ERROR,"x264_param_default_preset failed,errno:%d strerror:%s\n",errno,strerror(errno));
        return false;
    }
    
    convert_param(param);

    /* Open file for debug */
    m_fd = fopen(m_filepath,"w+");
    if( NULL == m_fd )
    {
        log_printf(LOG_LEVEL_ERROR,"open debug file failed,%s,errno:%d strerror:%s\n",m_filepath,errno,strerror(errno));
        return false;
    }

    /* Apply profile restrictions. */
    if( x264_param_apply_profile( &m_x264_param, "high" ) < 0 )
    {
        log_printf(LOG_LEVEL_ERROR,"x264_param_apply_profile failed,errno:%d strerror:%s\n",errno,strerror(errno));
        return false;
    }
    if( x264_picture_alloc( &m_picture, m_x264_param.i_csp, m_x264_param.i_width, m_x264_param.i_height ) < 0 )
    {
        log_printf(LOG_LEVEL_ERROR,"x264_picture_alloc failed,errno:%d strerror:%s\n",errno,strerror(errno));
        return false;
    }
    m_x264 = x264_encoder_open( &m_x264_param );
    if( !m_x264 )
    {
        log_printf(LOG_LEVEL_ERROR,"x264_encoder_open failed,errno:%d strerror:%s\n",errno,strerror(errno));
         x264_picture_clean( &m_picture );
         return false;
    }
    return true;
}
bool x264_encoder::Close()
{
    if( m_x264 )
    {
        int frame_size = 0;
        x264_nal_t *nal = NULL;
        int i_nal = 0;
        /* Flush delayed frames */
        while( x264_encoder_delayed_frames( m_x264 ) )
        {
            frame_size = x264_encoder_encode( m_x264, &nal, &i_nal, NULL, &m_picture_out );
            if( frame_size < 0 )
            {
                break;
            }
            else if( frame_size )
            {
                if( !fwrite( nal->p_payload, frame_size, 1, m_fd ) )
                    break;
            }
        }
        x264_encoder_close(m_x264 );
        x264_picture_clean( &m_picture );
    }
    if( NULL != m_fd )
    {
        fclose(m_fd);
        m_fd = NULL;
    }
    return true;
}
void x264_encoder::Encode(const void* p)
{
    if( NULL == m_x264 )
    {
        log_printf(LOG_LEVEL_ERROR,"x264 is not init!\n");
        return;
    }
    static int pts = 1;
    x264_nal_t *nal;
    int i_nal;
    unsigned char* in=(unsigned char*)p;
    memcpy(m_picture.img.plane[0],in, m_luma_size);
    memcpy(m_picture.img.plane[1],in+m_luma_size, m_chroma_size);
    memcpy(m_picture.img.plane[2],in+m_luma_size+m_chroma_size, m_chroma_size);
    m_picture.i_pts = pts;
    printf("m_luma_size:%d m_chroma_size*2:%d\n",m_luma_size,m_chroma_size*2);
    int frame_size = x264_encoder_encode( m_x264, &nal, &i_nal, &m_picture, &m_picture_out );
    if( frame_size < 0)
    {
        log_printf(LOG_LEVEL_ERROR,"encode one frame failed!\n");
    }
    else if( frame_size )
    {
        printf("one frame size :%d \n",frame_size);
        if( !fwrite( nal->p_payload,frame_size,1,m_fd) )
        {
            log_printf(LOG_LEVEL_ERROR," write to file %s failed! errno:%d,strerror:%s\n",m_filepath,errno,strerror(errno));
            return;
        }
    }
}
bool x264_encoder::convert_param(const x264_encoder_t param)
{
    m_x264_param.i_width = param.width;
    m_x264_param.i_height = param.height;
    switch( param.yuv )
    {
        case VIDEO_PIX_FMT_YUV420:
        {
            m_x264_param.i_csp = X264_CSP_I420;
            break;
        }
        default:m_x264_param.i_csp = X264_CSP_I420;
    }

    m_x264_param.b_vfr_input = 0;
    m_x264_param.b_repeat_headers = 1;
    m_x264_param.b_annexb = 1;
    m_x264_param.pf_log = x264_log_printf;

    m_luma_size = param.width * param.height;
    m_chroma_size = m_luma_size / 4;

    strcpy(m_filepath,param.filepath);
    strcat(m_filepath,"/test.h264");
    return true;
}

void x264_encoder::x264_log_printf(void* unused,int level,const char* psz_fmt,va_list arg)
{
    int log_level = 0;
    switch( level )
    {
        case X264_LOG_ERROR:
            log_level = LOG_LEVEL_ERROR;
            break;
        case X264_LOG_WARNING:
            log_level = LOG_LEVEL_WARNING;
            break;
        case X264_LOG_INFO:
            log_level = LOG_LEVEL_INFO;
            break;
        case X264_LOG_DEBUG:
            log_level = LOG_LEVEL_DEBUG;
            break;
        default:
            log_level = LOG_LEVEL_ERROR;
            break;
    }
    char buf[1024] = {0};
    va_list arg2;
    va_copy( arg2, arg );
    int length = vsnprintf( buf, sizeof(buf), psz_fmt, arg2 );
    va_end( arg2 );
    if( length > 0 )
    {
        log_printf(log_level,"<x264 library>%s",buf);
    }
}