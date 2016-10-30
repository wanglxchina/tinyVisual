#include "capture.h"
#include <linux/fb.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include "log.h"
#include "util.h"
#include "display_free_buffer.h"
#include "x264_encoder.h"

#define TV_VIDEO_WIDTH 640//640
#define TV_VIDEO_HEIGHT 480//480

display_free_buffer g_dfb;
x264_encoder g_x264_encoder;

static void process_image (const void * p){
//	g_dfb.processer(p);
   g_x264_encoder.Encode(p);
}

int main()
{
	//初始化log
	char local_path[255] = {0};
	getcwd(local_path,sizeof(local_path));
	strcat(local_path,"/log/");
	log_init(local_path);
	log_printf(LOG_LEVEL_INFO,"tinyVisual start!\n");
    //打开freebuffer的fd
    free_buffer_t fbt;
	fbt.width = TV_VIDEO_WIDTH;
	fbt.height = TV_VIDEO_HEIGHT;
	if( !g_dfb.Open(fbt) )
	{
		_my_assert(__func__,__LINE__,0);
        return -1;
	}
    log_printf(LOG_LEVEL_INFO,"freebuffer open and set success!\n");
    //打开编码器
    x264_encoder_t x264_encoder_param;
    x264_encoder_param.width = TV_VIDEO_WIDTH;
    x264_encoder_param.height = TV_VIDEO_HEIGHT;
    x264_encoder_param.yuv = VIDEO_PIX_FMT_YUV420;
    x264_encoder_param.filepath = local_path;
    if( !g_x264_encoder.Open(x264_encoder_param) )
    {
        log_printf(LOG_LEVEL_ERROR,"x264 encoder  open and set failed!\n");
        _my_assert(__func__,__LINE__,0);
        return -1;
    }
    log_printf(LOG_LEVEL_INFO,"x264_encoder open and set success!\n");
    //打开cam
	video_cap_format_t video_cap_format;
	video_cap_format.width = TV_VIDEO_WIDTH;
	video_cap_format.height = TV_VIDEO_HEIGHT;
	video_cap_format.pixelformat = VIDEO_PIX_FMT_YUV420;
	video_cap_format.isinterlaced = false;
	video_cap_format.v_ioMethod = IO_METHOD_MMAP;
	video_cap_format.framesPerSecond = 25;
	video_cap_format.processer = process_image;
    const char* dev_name = "/dev/video0";
    CaptureDevice device;
    if ( !device.Open(dev_name,video_cap_format) )
    {
		_my_assert(__func__,__LINE__,0);
        return -1;
    }
    log_printf(LOG_LEVEL_INFO,"device:%s open and set success!\n",dev_name);
    //开始采集
    if ( !device.Start() )
    {
		_my_assert(__func__,__LINE__,0);
        return -1;
    }
    log_printf(LOG_LEVEL_INFO,"device:%s start success!\n",dev_name);
    //等待
    usleep(20*1000*1000);
    //停止采集
    log_printf(LOG_LEVEL_INFO,"device:%s stop!\n",dev_name);
    device.Stop();
    //关闭cam
    log_printf(LOG_LEVEL_INFO,"device:%s close!\n",dev_name);
    device.Close();
    //关闭fb
    g_dfb.Close();
	log_printf(LOG_LEVEL_INFO,"close cam and leave!\n");
    //关闭x264编码器
    g_x264_encoder.Close();
	usleep(1000*1000);
    return 0;
}