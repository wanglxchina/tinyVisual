/*
file    : capture.h 
descripe: 根据官网的capture.c改写的C++版,部分流程改写
note    : 打开-->初始化-->开始-->循环采集-->停止-->逆初始化-->关闭
date    : 2016-08-21
author  : wanglx
*/
#ifndef _H_CAPTURE_H_
#define _H_CAPTURE_H_
#include <string>
#include <pthread.h>
typedef enum {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
} io_method;
struct v4l2buffer {
        void *                  start;
        size_t                  length;
};

typedef void(*VIDEO_DATA_PROCESSER)(const void *);
class CaptureDevice
{
public:
    CaptureDevice();
    ~CaptureDevice();
    bool Open(std::string deviceName = "");
    bool Start();
    bool Stop();
    void Close();
    void RegisteDataProcess(VIDEO_DATA_PROCESSER func);
    std::string GetLastError();
protected:
    bool Init();
    void Uinit();
    void loop();
    bool ReadFrame();
    bool InitRead(unsigned int bufferSize);
    bool InitMMap(unsigned int bufferSize);
    bool InitUserp(unsigned int bufferSize);
    static void* DataCaptureProc(void* param);
private:
    std::string             m_deviceName;
    std::string             m_lastError;
    io_method               m_ioMethod;
    int                     m_fd;
    bool                    m_needStop;
    bool                    m_captureState;
    v4l2buffer *            m_buffers;
    unsigned int            m_bufferCount;
    unsigned int            m_framesPerSecond;
    pthread_t               m_threadId;
    VIDEO_DATA_PROCESSER    ProcessImage;
};
#endif