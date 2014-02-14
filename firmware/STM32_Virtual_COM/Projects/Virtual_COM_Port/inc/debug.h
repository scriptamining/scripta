#ifndef __DEBUH_H_
#define __DEBUG_H_
/* debug.h
   2014/1/15
*/
#include <stdio.h>

#if defined(USB_DEBUG_PRINT)
extern char USB_PRINTF_BUF[256];
extern int USB_PRINTF_BUF_LENGHT;
#define PR_DEBUG(fmt,...) \
	USB_PRINTF_BUF_LENGHT = sprintf(USB_PRINTF_BUF,fmt, ##__VA_ARGS__); \
	UserToPMABufferCopy((unsigned char*)USB_PRINTF_BUF, ENDP1_TXADDR, USB_PRINTF_BUF_LENGHT); \
  SetEPTxCount(ENDP1, USB_PRINTF_BUF_LENGHT); \
  SetEPTxValid(ENDP1)

#elif defined(ITM_DEBUG_PRINT)
#define PR_DEBUG(fmt,...) \
	printf(fmt,##__VA_ARGS__)

#elif defined(UART1_DEBUG_PRINT)
#define PR_DEBUG(fmt,...) \
	printf(fmt,##__VA_ARGS__)
#define PR_BUFFER(buf,len) \
	USB_To_USART_Send_Data(buf,len)

#else
#define PR_DEBUG(fmt,...) 
#endif

#endif /*__DEBUG_H_*/
