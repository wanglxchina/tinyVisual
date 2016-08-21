#include "capture.h"
#include "tv_string.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <asm/types.h> 
#include <unistd.h>
#define DEFAULT_DEVICE_NAME "/dev/video0"

CaptureDevice::CaptureDevice():
m_deviceName(DEFAULT_DEVICE_NAME),
m_lastError(""),
m_ioMethod(IO_METHOD_MMAP),
m_fd(-1),
m_needStop(false),
m_captureState(false),
m_buffers(NULL),
m_bufferCount(0),
m_framesPerSecond(25),
m_threadId(0)
{
}
CaptureDevice::~CaptureDevice()
{
}

bool CaptureDevice::Open(std::string deviceName)
{
    //查询设备是否存在
    struct stat st;
    if (-1 == stat(deviceName.c_str(), &st))
    {
        printf("Cannot identify %s err:%d-->%s\n",
        deviceName.c_str(),errno,strerror(errno));
        return false;
    }
    //查询设备是否为字符型设备
    if(!S_ISCHR(st.st_mode))
    {
        printf ("%s is no device\n", deviceName.c_str());
    }
    //以可读写和非阻塞方式打开设备
    m_fd = open (deviceName.c_str(), O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == m_fd) 
        {
            printf ("Cannot open '%s': %d, %s\n",
            deviceName.c_str(), errno, strerror (errno));
            return false;
        }
    return Init();
}

bool CaptureDevice::Init()
{
    //获取制定设备支持的属性及操作
    struct v4l2_capability cap;
    if ( -1 == ioctl(m_fd,VIDIOC_QUERYCAP,&cap) )
    {
        if ( EINVAL == errno )
        {
            printf("%s is no v4l2 device\n",m_deviceName.c_str());
        }
        else
        {
            printf("VIDIOC_QUERYCAP failed\n");
        }
        return false;
    }
    //判断是否支持视频采集
    if ( !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) )
    {
        printf("%s is no video capture device\n",m_deviceName.c_str());
        return false;
    }
    //判断制定的IO方式是否支持
    switch( m_ioMethod )
    {
        case IO_METHOD_READ:
        {
            if ( !(cap.capabilities & V4L2_CAP_STREAMING) )
            {
                printf("%s does not support streaming i/o\n",m_deviceName.c_str());
                return false;
            }
        }
        break;
        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
        {
            if ( !(cap.capabilities & V4L2_CAP_STREAMING) )
            {
                printf("%s does not support streaming i/o\n",m_deviceName.c_str());
                return false;
            }
        }
        break;
        default:assert(0);
    }
    //查询并设置采集视频输入的裁剪和缩放能力RGB
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    memset(&cropcap,0,sizeof(cropcap));
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if ( 0 == ioctl (m_fd, VIDIOC_CROPCAP, &cropcap) )
    {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; // reset to default
        if ( -1 == ioctl (m_fd, VIDIOC_S_CROP, &crop) )
        {
            switch (errno) 
            {
                case EINVAL:
                   // Cropping not supported.
                   break;
                default:
                   // Errors ignored.
                   break;
            }
        }
    }
    else
    {
        // Errors ignored.
    }
    //设置视频格式(RGB转YUV编码，目标尺寸，YUV编码格式，是否交错)
    struct v4l2_format fmt;
	unsigned int min;
    memset(&fmt,0,sizeof(fmt));
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = 640;//cropcap.defrect.width;
    fmt.fmt.pix.height      = 480;//cropcap.defrect.height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;//V4L2_FIELD_INTERLACED;
    if ( -1 == ioctl(m_fd,VIDIOC_S_FMT,&fmt) )
    {
        printf("%s VIDIOC_S_FMT failed!errno:%d strerr:%s\n",m_lastError.c_str(),errno,strerror(errno));
        return false;
    }
    //设置YUV存储格式(驱动和应用程序交换数据,计算缓冲大小)
    min = fmt.fmt.pix.width * 2;//422存储一行的字节数
    if ( fmt.fmt.pix.bytesperline < min )
    {
        fmt.fmt.pix.bytesperline = min;
    }
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if ( fmt.fmt.pix.sizeimage < min )
    {
        fmt.fmt.pix.sizeimage = min;
    }
    bool ret = false;
    switch( m_ioMethod )
    {
        case IO_METHOD_READ:
		    ret = InitRead (fmt.fmt.pix.sizeimage);
		    break;

	    case IO_METHOD_MMAP:
		    ret = InitMMap (fmt.fmt.pix.sizeimage);
		    break;

	    case IO_METHOD_USERPTR:
		    ret = InitUserp (fmt.fmt.pix.sizeimage);
		    break;
    }
    if ( !ret )
    {
        return false;
    }
    //查询并设置设置每秒采集帧数
    struct v4l2_streamparm streamParm;
    memset(&streamParm,0,sizeof(streamParm));
    streamParm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if ( 0 == ioctl(m_fd,VIDIOC_G_PARM,&streamParm) )
    {
        if ( V4L2_CAP_TIMEPERFRAME == streamParm.parm.capture.capability )
        {
            streamParm.parm.capture.timeperframe.numerator = 1;
            streamParm.parm.capture.timeperframe.denominator = m_framesPerSecond;
            if ( -1 == ioctl(m_fd,VIDIOC_S_PARM,&streamParm) )
            {
                printf("%s VIDIOC_S_PARM failed! errno:%d strerr:%s.\n",m_deviceName.c_str(),errno,strerror(errno));
                return false;
            }
        }
        m_framesPerSecond = streamParm.parm.capture.timeperframe.denominator;
    }
    return true;
}

bool CaptureDevice::InitRead(unsigned int bufferSize)
{
    m_bufferCount = 1;
    m_buffers = (v4l2buffer*)calloc(m_bufferCount,sizeof(*m_buffers));
    if ( !m_buffers )
    {
        printf("calloc failed!\n");
        return false;
    }
    m_buffers[0].length = bufferSize;
    m_buffers[0].start = malloc(bufferSize);
    if ( !m_buffers[0].start )
    {
        printf("calloc failed!\n");
        free(m_buffers);
        m_buffers = NULL;
        return false;
    }
    return true;
}
bool CaptureDevice::InitMMap(unsigned int bufferSize)
{
    struct v4l2_requestbuffers req;
    memset(&req,0,sizeof(req));
    m_bufferCount   = 4;
    req.count       = m_bufferCount;
    req.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory      = V4L2_MEMORY_MMAP;
    if ( -1 == ioctl(m_fd, VIDIOC_REQBUFS, &req) )
    {
        if ( EINVAL == errno )
        {
            printf("%s does not support memory mapping\n",m_deviceName.c_str());
        } 
        else
        {
            printf("VIDIOC_REQBUFS error\n");
        }
        return false;
    }
    if ( req.count < 2 )
    {
        printf("Insufficient buffer memory on %s\n",m_deviceName.c_str());
        return false;
    }
    m_bufferCount = req.count;
    m_buffers = (v4l2buffer*)calloc(req.count,sizeof(*m_buffers));
    if ( !m_buffers )
    {
        printf("calloc failed!\n");
    }
    for ( unsigned int i = 0; i < req.count; ++i )
    {
        struct v4l2_buffer buf;
        memset(&buf,0,sizeof(buf));
        buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory  = V4L2_MEMORY_MMAP;
        buf.index   = i;
        if (-1 == ioctl(m_fd, VIDIOC_QUERYBUF, &buf) )
        {
            printf("VIDIOC_QUERYBUF failed\n");
            return false;
        }
        m_buffers[i].length = buf.length;
        m_buffers[i].start  = 
                    mmap(NULL,//start anywhere
                         buf.length,
                         PROT_READ | PROT_WRITE,//required
                         MAP_SHARED,//recommend
                         m_fd,
                         buf.m.offset);
        if ( MAP_FAILED == m_buffers[i].start )
        { 
            //TODO: release all start.
            cstr_printf(m_lastError,"mmap failed!\n");
            free(m_buffers);
            m_buffers = NULL;
            return false;
        }
    } 
    return true;
}
bool CaptureDevice::InitUserp(unsigned int bufferSize)
{
    struct v4l2_requestbuffers req;
    memset(&req,0,sizeof(req));
    m_bufferCount   = 4;
    req.count       = m_bufferCount;
    req.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory      = V4L2_MEMORY_USERPTR;
    if ( -1 == ioctl(m_fd, VIDIOC_REQBUFS, &req) ) 
    {
        if ( EINVAL == errno )
        {
            printf("%s does not support user pointer.\n",m_deviceName.c_str());
        }
        else
        {
            printf("VIDIOC_REQBUFS failed.\n");
        }
        return false;
    }
    m_bufferCount = req.count;
    m_buffers = (v4l2buffer*)calloc(req.count,sizeof(*m_buffers));
    if ( !m_buffers )
    {
        printf("calloc failed.\n");
        return false;
    }
    for ( unsigned int i = 0; i < req.count; ++i )
    {
        m_buffers[i].length  = bufferSize;
        m_buffers[i].start   = malloc(bufferSize);
        if( !m_buffers[i].start )
        {
            printf("malloc failed.\n");
            //TODO release all other m_buffers[x].start
            free(m_buffers);
            m_buffers = NULL;
            return false;
        }
    }
    return true;
}
bool CaptureDevice::Start()
{
    bool ret = true;
    unsigned int i = 0;
    enum v4l2_buf_type type;
    switch( m_ioMethod )
    {
        case IO_METHOD_READ:
        {
            //Nothing to do.
        }
        break;
        case IO_METHOD_MMAP:
        {
            for ( i = 0; i < m_bufferCount; ++i )
            {
                struct v4l2_buffer buf;
                memset(&buf,0,sizeof(buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;
                if ( -1 == ioctl(m_fd,VIDIOC_QBUF,&buf) )
                {
                    printf("%s VIDIOC_QBUF errno:%d strerr:%s",m_deviceName.c_str(),errno,strerror(errno));
                    ret = false;
                    break;
                }
             }   
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if ( -1 == ioctl (m_fd, VIDIOC_STREAMON, &type) )
	        {
                printf("%s VIDIOC_STREAMON errno:%d strerr:%s",m_deviceName.c_str(),errno,strerror(errno));
                ret = false;
                break;
            }
        }
        break;
        case IO_METHOD_USERPTR:
        {
            for ( i = 0; i < m_bufferCount; ++i )
            {
                struct v4l2_buffer buf;
                memset(&buf,0,sizeof(v4l2_buffer));
                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        		buf.memory      = V4L2_MEMORY_USERPTR;
        		buf.m.userptr	= (unsigned long) m_buffers[i].start;
			    buf.length      = m_buffers[i].length;
                if ( -1 == ioctl (m_fd, VIDIOC_QBUF, &buf) )
                {
                    printf("%s VIDIOC_QBUF errno:%d strerr:%s",m_deviceName.c_str(),errno,strerror(errno));
                    ret = false;
                    break;
                }
            }
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if ( -1 == ioctl (m_fd, VIDIOC_STREAMON, &type) )
	        {
                printf("%s VIDIOC_STREAMON errno:%d strerr:%s",m_deviceName.c_str(),errno,strerror(errno));
                ret = false;
                break;
            }
        }
        break;
        default:assert(0);
    }
    if ( ret )
    {
        m_needStop  = false;
        ret = pthread_create(&m_threadId,NULL,DataCaptureProc,this);
        if ( 0 != ret )
        {
            printf("pthread_create failed! ret:%d\n",ret);//EAGAIN or EINVAL
            ret = false;
        }
        ret = true;
    }
    else
    {
        printf("%s %d io set failed!errno:%d,strerr:%s\n",__func__,__LINE__,errno,strerror(errno));
    }
   
    return ret;
}

void CaptureDevice::loop()
{
    printf("Enter %s \n",__func__);
    m_captureState = true;
    while( !m_needStop )
    {
     //   printf(" caputre one frame\n");
        fd_set fds;
        struct timeval tv;
        int r;

        FD_ZERO(&fds);
        FD_SET(m_fd,&fds);

        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(m_fd + 1, &fds, NULL, NULL, &tv);
        if ( -1 == r )
        {
            if ( EINTR == errno )
            {
                continue;
            }
            printf(" select error !,errno:%d,strerr:%s\n",errno,strerror(errno));
        }  
        if ( 0 == r )
        {
            printf("select timeout,errno:%d,strerr:%s\n",errno,strerror(errno));
        }

        if ( !ReadFrame() )
        {
            break;
        }
        // EAGAIN - continue select loop.
    }
    m_needStop = false;
    m_captureState = false;
    printf("Leave %s \n",__func__);
}

bool CaptureDevice::ReadFrame()
{
    struct v4l2_buffer buf;
	unsigned int i;

	switch (m_ioMethod) 
    {
	    case IO_METHOD_READ:
        {
    	    if (-1 == read (m_fd, m_buffers[0].start, m_buffers[0].length))
            {
                if ( EAGAIN == errno )
                {
                    return true;
                } 
                else
                {
                    printf("IO_METHOD_READ read failed! errno:%d,strerr:%s\n",errno,strerror(errno));
                    return false;
                }
		    }
            ProcessImage (m_buffers[0].start);
        }
	    break;
	    case IO_METHOD_MMAP:
        {
            memset(&buf,0,sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            if ( -1 == ioctl (m_fd, VIDIOC_DQBUF, &buf) )  
            {
                if ( EAGAIN == errno )
                {
                    return true;
                } 
                else
                {
                    printf("IO_METHOD_MMAP read frame failed! errno:%d,strerr:%s\n",errno,strerror(errno));
                    return false;
                }
			}
            assert (buf.index < m_bufferCount);

	        ProcessImage (m_buffers[buf.index].start);

		    if ( -1 == ioctl (m_fd, VIDIOC_QBUF, &buf) )
            {
                printf("IO_METHOD_MMAP read frame failed! errno:%d,strerr:%s\n",errno,strerror(errno));
                return false;
            }
		}
		break;
	    case IO_METHOD_USERPTR:
		{
            memset(&buf,0,sizeof(buf));
    		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    		buf.memory = V4L2_MEMORY_USERPTR;

		    if ( -1 == ioctl (m_fd, VIDIOC_DQBUF, &buf) )  
            {
                if ( EAGAIN == errno )
                {
                    return true;
                } 
                else
                {
                    printf("IO_METHOD_MMAP read frame failed! errno:%d,strerr:%s\n",errno,strerror(errno));
                    return false;
                }
			}

		    for (i = 0; i < m_bufferCount; ++i)
            {
                if (buf.m.userptr == (unsigned long) m_buffers[i].start
			     && buf.length == m_buffers[i].length)
                 {
                    break;
                 }
            }
		    assert (i < m_bufferCount);

    		ProcessImage ((void *) buf.m.userptr);

            if ( -1 == ioctl (m_fd, VIDIOC_QBUF, &buf) )
            {
                printf("IO_METHOD_MMAP read frame failed! errno:%d,strerr:%s\n",errno,strerror(errno));
                return false;
            }
        }
		break;
	}

	return true;
}

bool CaptureDevice::Stop()
{
    m_needStop = true;
    if ( m_threadId )
    {
        pthread_join(m_threadId,NULL);
        close(m_threadId);
        m_threadId = 0;
    }

    enum v4l2_buf_type type;
	switch ( m_ioMethod ) 
    {
	    case IO_METHOD_READ:
        {
            //Nothing to do.
        }
		break;
	    case IO_METHOD_MMAP:
	    case IO_METHOD_USERPTR:
        {
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		    if ( -1 == ioctl (m_fd, VIDIOC_STREAMOFF, &type) )
            {
                printf("VIDIOC_STREAMOFF failed.errno:%d,strerr:%s",errno,strerror(errno));
                return false;
            } 
        }
		break;
	}
    return true;
}

void CaptureDevice::Uinit()
{
    unsigned int i;
	switch ( m_ioMethod )
    {
	    case IO_METHOD_READ:
        {
            free (m_buffers[0].start);
        }
        break;
	    case IO_METHOD_MMAP:
        {
            for (i = 0; i < m_bufferCount; ++i)
            {
                //取消参数start所指的映射内存起始地址
			    if (-1 == munmap (m_buffers[i].start, m_buffers[i].length))
                {
                    printf("munmap failed.errno:%d,strerr:%s",errno,strerror(errno));
                }
            }
        }
		break;
	    case IO_METHOD_USERPTR:
		{
            for (i = 0; i < m_bufferCount; ++i)
            {
                free (m_buffers[i].start);
            }
        }
        break;
	}
	free (m_buffers);
}

void CaptureDevice::Close()
{
    Uinit();
    if ( -1 == close (m_fd) )
	{
        printf("close failed.errno:%d,strerr:%s",errno,strerror(errno));
        assert(0);
    }
    m_fd = -1;
}

void* CaptureDevice::DataCaptureProc(void* param)
{
    printf("Enter %s \n",__func__);
    CaptureDevice* cp = (CaptureDevice*)param;
    cp->loop();
    return NULL;
}

void CaptureDevice::RegisteDataProcess(VIDEO_DATA_PROCESSER func)
{
    ProcessImage = func;
}