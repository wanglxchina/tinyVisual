#include "display_free_buffer.h"
#include <linux/fb.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "log.h"
display_free_buffer::display_free_buffer():
m_fbfd(-1),
m_fbp(NULL)
{
}
display_free_buffer::~display_free_buffer()
{
}
bool display_free_buffer::Open()
{
    m_fbfd = open("/dev/fb0", O_RDWR);
	if (m_fbfd==-1) {
		log_printf(LOG_LEVEL_ERROR,"Error: cannot open framebuffer device.errno:%d,strerror:%s\n",errno,strerror(errno));
		return false;
	}
    long screensize=0;
	if (-1==ioctl(m_fbfd, FBIOGET_FSCREENINFO, &cfinfo)) {
		log_printf(LOG_LEVEL_ERROR,"Error reading fixed information.\n");
		return false;
	}
	if (-1==ioctl(m_fbfd, FBIOGET_VSCREENINFO, &m_vinfo)) {
		log_printf(LOG_LEVEL_ERROR,"Error reading variable information.\n");
		return false;
	}
	screensize = m_vinfo.xres * m_vinfo.yres * m_vinfo.bits_per_pixel / 8;
    log_printf(LOG_LEVEL_INFO,"freebuf vinfo.xres:%d,vinfo.yres:%d ,imagesize:%ld\n",m_vinfo.xres,m_vinfo.yres,screensize);
	m_fbp = (char *)mmap(NULL,screensize,PROT_READ | PROT_WRITE,MAP_SHARED ,m_fbfd, 0);
	if ( m_fbp == MAP_FAILED ) {
		log_printf(LOG_LEVEL_ERROR,"Error: failed to map framebuffer device to memory.\n");
		return false;
	}
	memset(m_fbp, 0, screensize);
    return true;
}
bool display_free_buffer::Close()
{
    if (-1 == munmap(m_fbp, screensize)) {
		log_printf(LOG_LEVEL_INFO," Error: framebuffer device munmap() failed.\n");
		return false;
	}    
    close(m_fbfd);
    return true;
}
void display_free_buffer::processer(const void * p)
{

}