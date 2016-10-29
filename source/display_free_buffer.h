#ifndef _H_DISPLAY_FREE_BUFFER_H_
#define _H_DISPLAY_FREE_BUFFER_H_
#include <linux/fb.h>
class display_free_buffer
{
public:
    display_free_buffer();
    ~display_free_buffer();
    bool Open();
    bool Close();
    void processer(const void * p);
private:
    int     m_fbfd;
    char*   m_fbp;
    struct fb_var_screeninfo m_vinfo;
    struct fb_fix_screeninfo m_finfo;
}
#endif