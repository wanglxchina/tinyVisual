/*
file    : capture.h 
descripe: 根据官网的capture.c改写的C++版,部分流程改写
date    : 2016-08-21
author  : wanglx
note    :打开-->初始化-->开始-->循环采集-->停止-->逆初始化-->关闭
v4l2采集需要初始化步骤
1.获取设备的属性及支持的操作，VIDIOC_QUERYCAP
2.判断是否支持视屏采集，V4L2_CAP_VIDEO_CAPTURE
3.判定支持的IO方式(用户与内核交换数据的方式)，V4L2_CAP_STREAMING
4.设置设备恢复裁剪几缩放的默认值，VIDIOC_CROPCAP
5.设置视频的格式，视频高宽，YUV编码格式，是否交错
6.设置用户与内核交换数据的方式，计算缓冲大小，Read/Write方式，MMAP内存映射方式，USERPTR用户指针方式
7.设置每秒采集的帧数，VIDIOC_S_PARM
*/
#ifndef _H_CAPTURE_H_
#define _H_CAPTURE_H_
#include <string>
#include <pthread.h>
#include <map>
#include "video_header.h"
typedef enum {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR
} io_method;
struct v4l2buffer {
        void *                  start;
        size_t                  length;
};

typedef void(*VIDEO_DATA_PROCESSER)(const void *);
struct video_cap_format_t
{
    unsigned int width;
    unsigned int height;
    unsigned int pixelformat;
    bool isinterlaced;
    io_method v_ioMethod;
    unsigned int framesPerSecond;
    VIDEO_DATA_PROCESSER processer;
};
class CaptureDevice
{
public:
    CaptureDevice();
    ~CaptureDevice();
    bool Open(const std::string deviceName,const video_cap_format_t& format);
    bool Start();
    bool Stop();
    void Close();
    std::string GetLastError();
protected:
    bool Init();
    void Uinit();
    void loop();
    void SaveForamt(const video_cap_format_t& foramt);
    bool ReadFrame();
    bool InitRead(unsigned int bufferSize);
    bool InitMMap(unsigned int bufferSize);
    bool InitUserp(unsigned int bufferSize);
    static void* DataCaptureProc(void* param);
private:
    std::string             m_deviceName;
    std::string             m_lastError;
    int                     m_fd;
    bool                    m_needStop;
    bool                    m_captureState;
    v4l2buffer *            m_buffers;
    unsigned int            m_bufferCount;
    unsigned int            m_field;
    pthread_t               m_threadId;
    video_cap_format_t      m_format;
};
#endif