#ifndef _H_X264_ENCODER_H_H_
#define _H_X264_ENCODER_H_H_
#include <stdint.h>
#include "x264.h"
#include <stdio.h>
#include <unistd.h>
typedef struct x264_encoder_t
{
    unsigned int width;
    unsigned int height;
    unsigned int yuv;
    char* filepath;/*for debug*/
}x264_encoder_t;
class x264_encoder
{
public:
    x264_encoder();
    ~x264_encoder();
    bool Open(const x264_encoder_t param);
    bool Close();
    void Encode(const void* p);
private:
    bool convert_param(const x264_encoder_t param);
    static void x264_log_printf(void* unused,int level,const char* psz_fmt,va_list arg); 
    x264_param_t    m_x264_param;
    x264_picture_t  m_picture;
    x264_picture_t  m_picture_out;
    x264_t*         m_x264;
    int             m_luma_size;
    int             m_chroma_size;

    /*FOR DEBUG*/
    FILE*       m_fd;
    char        m_filepath[255];
};
#endif