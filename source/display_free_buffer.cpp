#include "display_free_buffer.h"
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "log.h"
#include "video_header.h"
#include "util.h"

inline int display_free_buffer::clip(int value, int min, int max) {
	return (value > max ? max : value < min ? min : value);
}
inline void display_free_buffer::show_One_Piexl(int location,int y,int u,int v)
{
	int r,g,b;
	r = (298 * y + 409 * v + 128) >> 8;
	g = (298 * y - 100 * u - 208 * v + 128) >> 8;
	b = (298 * y + 516 * u + 128) >> 8;

	m_fbp[ location + 0] = clip(b, 0, 255);
	m_fbp[ location + 1] = clip(g, 0, 255);
	m_fbp[ location + 2] = clip(r, 0, 255);    
	m_fbp[ location + 3] = 255;  
}
display_free_buffer::display_free_buffer():
m_fbfd(-1),
m_fbp(NULL),
m_screensize(0)
{
}
display_free_buffer::~display_free_buffer()
{
}
bool display_free_buffer::Open(free_buffer_t fbt)
{
	m_fbt = fbt;
    m_fbfd = open("/dev/fb0", O_RDWR);
	if (m_fbfd==-1) {
		log_printf(LOG_LEVEL_ERROR,"Error: cannot open framebuffer device.errno:%d,strerror:%s\n",errno,strerror(errno));
		return false;
	}
	if (-1==ioctl(m_fbfd, FBIOGET_FSCREENINFO, &m_finfo)) {
		log_printf(LOG_LEVEL_ERROR,"Error reading fixed information.\n");
		return false;
	}
	if (-1==ioctl(m_fbfd, FBIOGET_VSCREENINFO, &m_vinfo)) {
		log_printf(LOG_LEVEL_ERROR,"Error reading variable information.\n");
		return false;
	}
	m_screensize = m_vinfo.xres * m_vinfo.yres * m_vinfo.bits_per_pixel / 8;
    log_printf(LOG_LEVEL_INFO,"freebuf vinfo.xres:%d,vinfo.yres:%d ,imagesize:%ld\n",m_vinfo.xres,m_vinfo.yres,m_screensize);
	m_fbp = (char *)mmap(NULL,m_screensize,PROT_READ | PROT_WRITE,MAP_SHARED ,m_fbfd, 0);
	if ( m_fbp == MAP_FAILED ) {
		log_printf(LOG_LEVEL_ERROR,"Error: failed to map framebuffer device to memory.\n");
		return false;
	}
	memset(m_fbp, 0, m_screensize);
    return true;
}
bool display_free_buffer::Close()
{
    if (-1 == munmap(m_fbp, m_screensize)) {
		log_printf(LOG_LEVEL_INFO," Error: framebuffer device munmap() failed.\n");
		return false;
	}    
    close(m_fbfd);
    return true;
}
void display_free_buffer::processer(const void * p)
{
	unsigned char* in=(unsigned char*)p;
	int width=m_fbt.width;
	int height=m_fbt.height;
	if( m_fbt.yuv == VIDEO_PIX_FMT_YUV420 )
	{
		const long len = width * height;
		unsigned char* yData = in;
    	unsigned char* uData = &in[len];
    	unsigned char* vData = &in[len*5/4];
		long location=0;

		int yIdx,uIdx,vIdx;
		for (int i = 0;i < height;i++)
		{
        	for (int j = 0;j < width;j++)
			{
            	yIdx = i * width + j;
            	uIdx = (i/2) * (width/2) + (j/2);
            	vIdx = uIdx;

				location = (j + 100 + m_vinfo.xoffset) * (m_vinfo.bits_per_pixel/8) +
					(i + 100 + m_vinfo.yoffset) * m_finfo.line_length;

				show_One_Piexl(location,(int)yData[yIdx],(int)uData[uIdx]-128,(int)vData[vIdx]-128);
			//	show_One_Piexl(location,(int)yData[yIdx],0,0);
			}
		}
	} 
	else if( m_fbt.yuv == VIDEO_PIX_FMT_YUV422 )
	{
		int x,y,j;
		int y0,u,y1,v,r,g,b;
		long location=0;

		for ( y = 100; y < height + 100; ++y) {
			for (j = 0, x=100; j < width * 2 ; j += 4,x +=2) {

				location = (x+m_vinfo.xoffset) * (m_vinfo.bits_per_pixel/8) +
					(y+m_vinfo.yoffset) * m_finfo.line_length;

				y0 = in[j];
				u = in[j + 1] - 128;                
				y1 = in[j + 2];        
				v = in[j + 3] - 128;        

				r = (298 * y0 + 409 * v + 128) >> 8;
				g = (298 * y0 - 100 * u - 208 * v + 128) >> 8;
				b = (298 * y0 + 516 * u + 128) >> 8;

				m_fbp[ location + 0] = clip(b, 0, 255);
				m_fbp[ location + 1] = clip(g, 0, 255);
				m_fbp[ location + 2] = clip(r, 0, 255);    
				m_fbp[ location + 3] = 255;    

				r = (298 * y1 + 409 * v + 128) >> 8;
				g = (298 * y1 - 100 * u - 208 * v + 128) >> 8;
				b = (298 * y1 + 516 * u + 128) >> 8;

				m_fbp[ location + 4] = clip(b, 0, 255);
				m_fbp[ location + 5] = clip(g, 0, 255);
				m_fbp[ location + 6] = clip(r, 0, 255);    
				m_fbp[ location + 7] = 255;    
			}
			in += width * 2;
		}
	}//else if( m_fbt.yuv = VIDEO_PIX_FMT_YUV422 )
}