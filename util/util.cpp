#include "util.h"
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include "log.h"
void _my_assert(const char* func, int line, int value)
{
	if (value == 0)
	{
		log_printf(LOG_LEVEL_ERROR,"******FETAL ERROR******, before we assert here. %s:%d:%d *******\n", func, line, errno);
		log_printf(LOG_LEVEL_ERROR,"******FETAL ERROR******, before we assert here. %s:%d:%d *******\n", func, line, errno);
		log_printf(LOG_LEVEL_ERROR,"******FETAL ERROR******, before we assert here. %s:%d:%d *******\n", func, line, errno);
		log_printf(LOG_LEVEL_ERROR,"******FETAL ERROR******, before we assert here. %s:%d:%d *******\n", func, line, errno);
		log_printf(LOG_LEVEL_ERROR,"******FETAL ERROR******, before we assert here. %s:%d:%d *******\n", func, line, errno);
		log_printf(LOG_LEVEL_ERROR,"******FETAL ERROR******, before we assert here. %s:%d:%d *******\n", func, line, errno);
		log_printf(LOG_LEVEL_ERROR,"******FETAL ERROR******, before we assert here. %s:%d:%d *******\n", func, line, errno);
		log_printf(LOG_LEVEL_ERROR,"******FETAL ERROR******, before we assert here. %s:%d:%d *******\n", func, line, errno);
        usleep(1000*1000);
        exit(0);
    }
}