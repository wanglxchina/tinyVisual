#ifndef _H_DISPLAY_SDL2_H_H_
#define _H_DISPLAY_SDL2_H_H_
#include "SDL.h"
class display_sdl2
{
public:
    display_sdl2();
    ~display_sdl2();
    bool Open();
    bool Close();
    void processer(const void * p);    
};
#endif