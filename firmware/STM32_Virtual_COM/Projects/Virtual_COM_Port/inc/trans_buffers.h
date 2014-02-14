#ifndef __TRANS_BUFFERS_H_
#define __TRANS_BUFFERS_H_

#define UART_TRANS_BUFFER_SIZE 64
#define USB_TRANS_BUFFER_SIZE 2048

struct TRANS_BUFFER {
	uint8_t * buffer;
	uint32_t size;
	uint32_t in;
	uint32_t out;
};

void init_trans_buffer(struct TRANS_BUFFER * tb, uint8_t * buffer, uint32_t size);
void reset_trans_buffer(struct TRANS_BUFFER *tb);
int put_trans_buffer(struct TRANS_BUFFER *tb, uint8_t * buffer, uint32_t length);
int get_trans_buffer(struct TRANS_BUFFER *tb, uint8_t * buffer, uint32_t max_length);
int get_trans_buffer_cmd(struct TRANS_BUFFER *tb, uint8_t * buffer);
void flush_trans_buffer(struct TRANS_BUFFER *tb);
int trans_buffer_out_length(struct TRANS_BUFFER *tb);
void copy_buffer(u8 * dst, u8 * src, u32 length);
void trans_buf_to_usb(u8 *buf, u32 length);
void trans_buf_to_uart(USART_TypeDef* USARTx,u8 *buf, u32 length);
u8 get_next_n_out(struct TRANS_BUFFER *tb, int n);
int trans_buffer_has_cmd(struct TRANS_BUFFER *tb);
void USART_to_trans_buffer(USART_TypeDef* USARTx, struct TRANS_BUFFER *tb);

extern struct TRANS_BUFFER usb_trans_buffer;
extern struct TRANS_BUFFER uart_trans_buffer[COMn];

#endif /*__TRANS_BUFFERS_H_*/
