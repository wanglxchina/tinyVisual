#ifndef _H_DISPLAY_FREE_BUFFER_H_
#define _H_DISPLAY_FREE_BUFFER_H_
#include <linux/fb.h>
typedef struct free_buffer_t
{
    unsigned int width;
    unsigned int height;
    unsigned int yuv;
}free_buffer_t;
class display_free_buffer
{
public:
    display_free_buffer();
    ~display_free_buffer();
    bool Open(free_buffer_t fbt);
    bool Close();
    void processer(const void * p);
private:
inline int clip(int value, int min, int max);
inline void show_One_Piexl(int location,int y,int u,int v);
    int     m_fbfd;
    char*   m_fbp;
    long    m_screensize;
    struct fb_var_screeninfo m_vinfo;
    struct fb_fix_screeninfo m_finfo;
    struct free_buffer_t m_fbt;
};
#endif