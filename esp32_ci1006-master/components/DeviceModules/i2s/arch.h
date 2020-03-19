#ifndef _ARCH_H
#define _ARCH_H
#include <stdarg.h> 
void msg_output(char* str,...);
unsigned int  USTIMER_get(void);
int uart_get(int ch, char* buf);

#ifdef WIN32
#define restrict  
#endif
#define trace(fmt,args,...) msg_output("trace:{%s} [%s] <%d>..."fmt"\n\r", __FILE__, __FUNCTION__, __LINE__,##args)

#endif
