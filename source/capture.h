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

class CaptureDevice
{
public:
    CaptureDevice();
    ~CaptureDevice();
    bool Open(std::string deviceName = "");
    bool Start();
    bool Stop();
    void Close();
    std::string GetLastError();
protected:
    bool Init();
    void Uinit();
    void loop();
    bool ReadFrame();
    bool InitRead(unsigned int bufferSize);
    bool InitMMap(unsigned int bufferSize);
    bool InitUserp(unsigned int bufferSize);
    void ProcessImage(const void *  p);
    static void* DataCaptureProc(void* param);
private:
    std::string     m_deviceName;
    std::string     m_lastError;
    io_method       m_ioMethod;
    int             m_fd;
    bool            m_needStop;
    bool            m_captureState;
    v4l2buffer *    m_buffers;
    unsigned int    m_bufferCount;
    unsigned int    m_framesPerSecond;
    pthread_t       m_threadId;
};
#endif