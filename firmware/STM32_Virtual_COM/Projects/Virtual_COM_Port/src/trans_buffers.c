/* trans_buffers.c
    start: 2014/1/16
*/
#include "stm32f10x_usart.h"
#include "platform_config.h"
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "trans_buffers.h"
#include "debug.h"
#include "miner.h"

extern LINE_CODING linecoding;

struct TRANS_BUFFER usb_trans_buffer;
struct TRANS_BUFFER uart_trans_buffer[COMn];

void init_trans_buffer(struct TRANS_BUFFER * tb, uint8_t * buffer, uint32_t size)
{
	tb->buffer = buffer;
	tb->size = size;
	tb->in = 0;
	tb->out = 0;
}

void reset_trans_buffer(struct TRANS_BUFFER *tb)
{
	tb->in =0;
	tb->out = 0;
}

int put_trans_buffer(struct TRANS_BUFFER *tb, uint8_t * buffer, uint32_t length)
{
	uint8_t *buf;
	int i,j;
	
	buf = (uint8_t *)tb->buffer + tb->in;
	
	if (length >= tb->size)
	{
		/* if length > size, eventhough put the buffer,
		it will be in=out, the get_trans_buffer can't get anything,
		so do nothing here */
		return -1;
/*		
		j = tb->size - tb->in;
		for (i=0; i<j; i++)
			buf[i] = buffer[i];
		for (i=0; i<tb->in; i++)
			tb->buffer[i] = buffer[j+i];
*/			
	}
	else if ((tb->in + length) < tb->size)
	{
		tb->in += length;
		for (i=0; i < length; i++)
		{
			buf[i] = buffer[i];
		}
	}
	else /*roll back*/
	{
		for (i=0;i < (tb->size - tb->in); i++)
			buf[i] = buffer[i];
		
		j = tb->size - tb->in;	
		tb->in = tb->in + length - tb->size;
		for (i=0; i < tb->in; i++)
			tb->buffer[i] = buffer[j+i];
	}

	return tb->in;
}

u8 get_next_n_out(struct TRANS_BUFFER *tb, int n)
{
	if (tb->out + n < tb->size)
	{
		return tb->buffer[tb->out + n];
	}
	else /*roll back*/
	{
		return tb->buffer[tb->out + n - tb->size];
	}
}

int trans_buffer_has_cmd(struct TRANS_BUFFER *tb)
{
	int n;
	int length;

	length = trans_buffer_out_length(tb);
	if (length < 5) return 0;

	for(n = 3; n < length - 1; n++)
	{
		if (get_next_n_out(tb,n) == HEADER_55 
			&& get_next_n_out(tb,n+1) == HEADER_AA)
			return n;
	}
	return 0;
}

int get_trans_buffer_cmd(struct TRANS_BUFFER *tb, uint8_t * buffer)
{
	int length = trans_buffer_has_cmd(tb);
	if (length > 0)
		get_trans_buffer(tb, buffer, length);
	return length;
}

int trans_buffer_out_length(struct TRANS_BUFFER *tb)
{
	if (tb->out == tb->in)
		return 0;
	else if (tb->out < tb->in)
		return (tb->in - tb->out);
	else
		return (tb->in + tb->size -tb->out);
}

void flush_trans_buffer(struct TRANS_BUFFER *tb)
{
	tb->out = tb->in;
}

int get_trans_buffer(struct TRANS_BUFFER *tb, uint8_t * buffer, uint32_t max_length)
{
	uint8_t *buf;
	int length = 0;
	int i;

	/* no more data */
	if (tb->out == tb->in)
		return 0;
		
	buf = tb->buffer + tb->out;
	if (tb->out < tb->in)
	{
		length = tb->in - tb->out;
		if (length > max_length) length = max_length;
		for (i=0; i<length; i++)
			buffer[i] = buf[i];
	}
	else /* roll back */
	{
		if (max_length <= tb->size - tb->out)
		{
			length = max_length;
			for (i = 0; i<length; i++)
				buffer[i] = buf[i];
		}
		else {
			length = tb->size - tb->out;
			
			for (i = 0; i < (tb->size - tb->out); i++)
				buffer[i] = buf[i];
			
			if ((max_length-length) < tb->in){
				for (i = 0; i < (max_length + tb->out - tb->size); i++)
					buffer[length+i] = tb->buffer[i];
				length = max_length;
			}
			else {
				for (i = 0; i < tb->in; i++)
					buffer[length+i] = tb->buffer[i];
				length += tb->in;
			}
		}
	}
	//tb->out = tb->in;
	tb->out += length;
	/*roll back*/
	if (tb->out >= tb->size)
		tb->out -= tb->size;

	//PR_DEBUG("in = %d, out=%d, length=%d\n",tb->in, tb->out, length);
	return length;
}

void copy_buffer(u8 * dst, u8 * src, u32 length)
{
	int i;
	for (i = 0; i<length; i++)
		dst[i] = src[i];
}

void trans_buf_to_usb(u8 *buf, u32 length)
{
    UserToPMABufferCopy(buf, ENDP1_TXADDR, length);
    SetEPTxCount(ENDP1, length);
    SetEPTxValid(ENDP1); 
}

void trans_buf_to_uart(USART_TypeDef* USARTx,u8 *buf, u32 length)
{
  u32 i;
  
  for (i = 0; i < length; i++)
  {
    USART_SendData(USARTx, *(buf + i));
    while(USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET); 
  }  	
}

/*******************************************************************************
* Function Name  : UART_To_trans_buffer.
* Description    : send the received data from UART 0 to USB.
* Input          : None.
* Return         : none.
*******************************************************************************/
void USART_to_trans_buffer(USART_TypeDef* USARTx, struct TRANS_BUFFER *tb)
{
  if (linecoding.datatype == 7)
  {
    tb->buffer[tb->in] = USART_ReceiveData(USARTx) & 0x7F;
  }
  else if (linecoding.datatype == 8)
  {
     tb->buffer[tb->in] = USART_ReceiveData(USARTx);
  }
  
  tb->in++;
  
  /* To avoid buffer overflow */
  if(tb->in == tb->size)
  {
    tb->in = 0;
  }
}