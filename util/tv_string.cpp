#include "tv_string.h"
#include <stdarg.h>

void cstr_printf(std::string& target,const char* format,...)
{
    char tmp[512] = {0};
    va_list arg_ptr;
    va_start(arg_ptr,format);
    snprintf(tmp,sizeof(tmp)-1,format,arg_ptr);
    target = tmp;
}