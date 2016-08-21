#include "capture.h"
#include <linux/fb.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
static int fbfd = -1;
static char *fbp=NULL;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;


inline int clip(int value, int min, int max) {
	return (value > max ? max : value < min ? min : value);
}
static void process_image (const void * p){


   // printf("one frame be capture,YUV --> RGB\n");
	//ConvertYUVToRGB32

	unsigned char* in=(unsigned char*)p;
	int width=640;
	int height=480;
	int istride=1280;
	int x,y,j;
	int y0,u,y1,v,r,g,b;
	long location=0;

	for ( y = 100; y < height + 100; ++y) {
		for (j = 0, x=100; j < width * 2 ; j += 4,x +=2) {

			location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
				(y+vinfo.yoffset) * finfo.line_length;

			y0 = in[j];
			u = in[j + 1] - 128;                
			y1 = in[j + 2];        
			v = in[j + 3] - 128;        

			r = (298 * y0 + 409 * v + 128) >> 8;
			g = (298 * y0 - 100 * u - 208 * v + 128) >> 8;
			b = (298 * y0 + 516 * u + 128) >> 8;

			fbp[ location + 0] = clip(b, 0, 255);
			fbp[ location + 1] = clip(g, 0, 255);
			fbp[ location + 2] = clip(r, 0, 255);    
			fbp[ location + 3] = 255;    

			r = (298 * y1 + 409 * v + 128) >> 8;
			g = (298 * y1 - 100 * u - 208 * v + 128) >> 8;
			b = (298 * y1 + 516 * u + 128) >> 8;

			fbp[ location + 4] = clip(b, 0, 255);
			fbp[ location + 5] = clip(g, 0, 255);
			fbp[ location + 6] = clip(r, 0, 255);    
			fbp[ location + 7] = 255;    
		}
		in +=istride;
	}
}

int main()
{
    //打开freebuffer的fd
    fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd==-1) {
		printf("Error: cannot open framebuffer device.errno:%d,strerror:%s\n",errno,strerror(errno));
		return -1;
	}
    long screensize=0;
	if (-1==ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
		printf("Error reading fixed information.\n");
		return -1;
	}
	if (-1==ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
		printf("Error reading variable information.\n");
		return -1;
	}
	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    printf("freebuf vinfo.xres:%d,vinfo.yres:%d ,imagesize:%ld\n",vinfo.xres,vinfo.yres,screensize);
	fbp = (char *)mmap(NULL,screensize,PROT_READ | PROT_WRITE,MAP_SHARED ,fbfd, 0);
	if ( fbp == MAP_FAILED ) {
		printf("Error: failed to map framebuffer device to memory.\n");
		return -1;
	}
	memset(fbp, 0, screensize);
    printf("freebuffer open and set success!\n");
    //打开cam
    const char* dev_name = "/dev/video0";
    CaptureDevice device;
    if ( !device.Open(dev_name) )
    {
        return -1;
    }
    device.RegisteDataProcess(process_image);
    printf("device:%s open and set success!\n",dev_name);
    //开始采集
    if ( !device.Start() )
    {
        return -1;
    }
    printf("device:%s start success!\n",dev_name);
    //等待
    usleep(20*1000*1000);
    //停止采集
    printf("device:%s stop!\n",dev_name);
    device.Stop();
    //关闭cam
    printf("device:%s close!\n",dev_name);
    device.Close();
    //关闭fb
    if (-1 == munmap(fbp, screensize)) {
		printf(" Error: framebuffer device munmap() failed.\n");
		return -1;
	}    
    close(fbfd);
    return 0;
}