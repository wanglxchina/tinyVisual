//#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
int test_video_version(int dev_fd)
{
    int ret = 0;
   char dummy[256];

    if (-1 != ioctl(dev_fd,VIDIOC_QUERYCAP,dummy)) {
        ret = 2;
    }
    return ret;
}
int main()
{
    const char* dev_name = "/dev/video0";
    int cam_fd = -1;

    cam_fd = open(dev_name,O_RDWR|O_NONBLOCK);
    if( cam_fd == -1 )
    {
        printf("open %s failure !\n",dev_name);
        return -1;
    }
    switch( test_video_version(cam_fd) )
    {
        case 0:
            printf("%s:%d is not v4l device\n",dev_name,cam_fd);
            break;
        case 2:
            printf("%s:%d is v4l2 device\n",dev_name,cam_fd);
            break;
    }
    return 0;
}