/* miner.c
    2014/1/8
*/
#include "hw_config.h"
#include "trans_buffers.h"
#include "miner.h"
#include "debug.h"
#include "string.h"
#include "delay.h"

#if defined(USE_USB_AS_HOST_IF)
#define trans_to_host(buf,len) trans_buf_to_usb(buf,len)
#elif defined(USE_UART1_AS_HOST_IF)
#define trans_to_host(buf,len) trans_buf_to_uart(EVAL_COM1, buf, len)
#endif

u8 cmd_buffer[CMD_MAX_LENGTH];
u8 report_buffer[REPORT_MAX_LENGTH];
u32 Max_Chip_Num = GC3355_NUM;
u32 btc_task_fifo_mode = 0;
u32 btc_task_fifo_start = 0;
BTC_TASK_CMD btc_task;
TASK_FIFO btc_task_fifo = {
	{0},
	BTC_TASK_FIFO_LEN + 1,
	0,
	0,
	0
};
LTC_TASK_CMD ltc_task;
MCU_CMD mcu_cmd;
u8 fw_ver[4]={FW_VER_LL,FW_VER_LH,FW_VER_HL,FW_VER_HH};
TASK_NONCE btc_nonce = {{0x55,0x10,0,0},0,0};
TASK_NONCE ltc_nonce = {{0x55,0x20,0,0},0,0};
volatile uint32_t ltc_com_sel = 0;

//#define MINER_CMD_DEBUG
#ifdef MINER_CMD_DEBUG
#define DBG_CMD(string,buf,len) \
		print_cmd_asc(string, buf, len)
#else
#define DBG_CMD(string,buf,len) 
#endif

#ifdef MINER_CMD_DEBUG
static void print_cmd_asc(const char *name, u8 *cmd, u32 len)
{
	int i;
	PR_DEBUG("%s",name);
	for (i=0; i<len; i++)
		PR_DEBUG("%02X ",cmd[i]);
	PR_DEBUG("\n");
}
#endif
extern USART_TypeDef* COM_USART[COMn];
static u32 get_task_fifo_free(TASK_FIFO *tf);
static u32 get_task_fifo_length(TASK_FIFO *tf);
static void trans_ltc_cmd(u8 *cmd, u32 cmd_len);
static void trans_btc_cmd(u8 *cmd, u32 cmd_len);

static u32 reset_task_fifo(TASK_FIFO * tf)
{
	tf->in = 0;
	tf->out = 0;
}

static u32 get_task_fifo(TASK_FIFO * tf, BTC_TASK_CMD *btc_task)
{
	if (tf->in == tf->out)
		return 0;
	memcpy((u8 *)btc_task, (u8 *)&tf->task[tf->out], sizeof(BTC_TASK_CMD));
	tf->out ++;
	if (tf->out == tf->size)
		tf->out = 0;
	return 1;
}

static u32 put_task_fifo(TASK_FIFO * tf, BTC_TASK_CMD *btc_task)
{
	if (tf->in == (tf->size - 1) && tf->out == 0) return 0;
	if ((tf->in+1) == tf->out) return 0;
	memcpy((u8 *)&tf->task[tf->in], (u8 *)btc_task, sizeof(BTC_TASK_CMD));
	tf->in ++;
	if (tf->in == tf->size)
		tf->in = 0;
	return 1;
}

static u32 get_task_fifo_free(TASK_FIFO *tf)
{
	if (tf->in == tf->out) return tf->size -1;
	if (tf->out > tf->in)
		return (tf->out - tf->in -1);
	else
		return (tf->out + tf->size - tf->in - 1);
}

static u32 get_task_fifo_length(TASK_FIFO *tf)
{
	if (tf->in == tf->out) return 0;
	if (tf->in > tf->out)
		return (tf->in - tf->out);
	else
		return (tf->in + tf->size - tf->out);
}

static void report_ltc_nonce(u32 nonce)
{
	ltc_nonce.nonce = nonce;
	ltc_nonce.taskid = ltc_task.taskid;
	trans_to_host((u8*)&ltc_nonce,sizeof(ltc_nonce));
	led0_revert();
}

static void report_btc_nonce(u32 nonce)
{
	btc_nonce.nonce = nonce;
	btc_nonce.taskid = btc_task.taskid;
	trans_to_host((u8*)&btc_nonce,sizeof(btc_nonce));
	led0_revert();
	
	if ((btc_task_fifo_mode == 1) 
		&& get_task_fifo(&btc_task_fifo, &btc_task))
		trans_btc_cmd((u8 *)&btc_task, sizeof(BTC_TASK_CMD) - 4);
}

static void trans_btc_cmd(u8 *cmd, u32 cmd_len)
{
	DBG_CMD("btc cmd<<<",cmd, cmd_len);
	//PR_DEBUG("btc cmd %d\n", cmd_len);
	trans_buf_to_uart(BTC_COM1, cmd, cmd_len);
#if defined(USE_STM3210E_EVAL)
	trans_buf_to_uart(BTC_COM2, cmd, cmd_len);
#endif	
}

static void trans_ltc_init_nonce(void)
{
	int sub;
	unsigned long step,base,top;
	LTC_NONCE_INIT ln;
	static u32 nonce_min;
	static u32 nonce_max;

	//if (nonce_min == ltc_task.nonce_min 
	//	&& nonce_max == ltc_task.nonce_max)
	//	return ;

	//nonce_min =  ltc_task.nonce_min;
	//nonce_max =  ltc_task.nonce_max;

	ln.header[0] = HEADER_55;
	ln.header[1] = HEADER_AA;
	step =  (ltc_task.nonce_max - ltc_task.nonce_min)/Max_Chip_Num;
	base =  ltc_task.nonce_min;
	top =  ltc_task.nonce_max;
	for(sub=0;sub<Max_Chip_Num;sub++)
	{
		ln.header[2]=(LTC|sub);
		ln.header[3]=0x23;
		ln.nonce_min = base + sub* step;
		ln.nonce_max = base + (sub+1)* step - 1;
		if(sub==Max_Chip_Num-1) 
			ln.nonce_max = top;
		delay_us(CMD_WAIT_TIME);
		trans_ltc_cmd((u8 *)&ln, sizeof(LTC_NONCE_INIT));
	}
}


static void trans_ltc_cmd(u8 *cmd, u32 cmd_len)
{
	DBG_CMD("ltc cmd<<<",cmd, cmd_len);
	//PR_DEBUG("ltc cmd %d\n", cmd_len);
	if (ltc_com_sel == 0) {
		trans_buf_to_uart(BTC_COM1, cmd, cmd_len);
#if defined(USE_STM3210E_EVAL)
		trans_buf_to_uart(BTC_COM2, cmd, cmd_len);
#endif	
	}
	else {
		trans_buf_to_uart(LTC_COM1, cmd, cmd_len);
#if defined(USE_STM3210E_EVAL)
		trans_buf_to_uart(LTC_COM2, cmd, cmd_len);
#endif			
	}
}

/* trans cpm cmds to gc3355 chips by usart channel*/
static void trans_cpm_cmd(u8 * cmd, u32 cmd_len)
{
	DBG_CMD("cpm cmd<<<",cmd, cmd_len);
	//PR_DEBUG("cpm cmd %d\n", cmd_len);
	trans_buf_to_uart(BTC_COM1, cmd, cmd_len);
#if defined(USE_STM3210E_EVAL)
	trans_buf_to_uart(BTC_COM2, cmd, cmd_len);
#endif	
}

static void process_btc_cmd(u8 *cmd, u32 cmd_len)
{
	//DBG_CMD("btc cmd<<<",cmd, cmd_len);
	if (cmd_len < sizeof(BTC_TASK_CMD) - 4)
		trans_btc_cmd(cmd, cmd_len);
		
	else if  (cmd_len == sizeof(BTC_TASK_CMD)) {
		if (btc_task_fifo_mode == 1) {
			if (btc_task_fifo_start == 0) {
				btc_task_fifo_start = 1;
				memcpy((u8 *)&btc_task, cmd, cmd_len);
				trans_btc_cmd(cmd, cmd_len - 4);
			}
			put_task_fifo(&btc_task_fifo, (BTC_TASK_CMD *)cmd);
		}
		else {
			btc_task_fifo_start = 0;
			memcpy((u8 *)&btc_task, cmd, cmd_len);
			trans_btc_cmd(cmd, cmd_len - 4);			
		}
	}
}

static void process_ltc_cmd(u8 *cmd, u32 cmd_len)
{
	//DBG_CMD("ltc cmd<<<",cmd, cmd_len);
	if (cmd_len < sizeof(LTC_TASK_CMD) - 12) {
		trans_ltc_cmd(cmd, cmd_len);
		/* gc3355 reg 28, set ltc uart select */
		if(cmd[2]==0x1f&&cmd[3]==0x28)
		{
			if((ltc_com_sel==0) && (cmd[4]&0x04)) {
				PR_DEBUG("ltc select uart\n");
				ltc_com_sel = 1;
			}
		}
	}
	/* ltc task */
	else if  (cmd_len == sizeof(LTC_TASK_CMD)) {
		memcpy((u8 *)&ltc_task, cmd, cmd_len);
		trans_ltc_cmd(cmd, sizeof(LTC_TASK_CMD) - 12);
		trans_ltc_init_nonce();
	}
}

static void process_cpm_cmd(u8 *cmd, u32 cmd_len)
{
	//DBG_CMD("mcu cmd<<<",cmd, cmd_len);
	trans_btc_cmd(cmd, cmd_len);
}

static void process_mcu_cmd(u8 *cmd, u32 cmd_len)
{
	//DBG_CMD("mcu cmd<<<",cmd, cmd_len);
	memcpy((u8 *)&mcu_cmd, cmd, sizeof(mcu_cmd));
	if(mcu_cmd.header[3]==0x01)//read  reg
	{
		if(mcu_cmd.reg_len>0)
		{
			trans_to_host((u8 *)&mcu_cmd.reg_addr, 4);
			trans_to_host((u8*)mcu_cmd.reg_addr,mcu_cmd.reg_value);
		}
	}
	else if(mcu_cmd.header[3]==0x02)//write reg
	{
		u32 *pReg;
		pReg = (u32 *)(mcu_cmd.reg_addr&0xffFFfffC);
		*pReg = mcu_cmd.reg_value;
		trans_to_host((u8 *)&mcu_cmd.reg_addr, 4);
		trans_to_host((u8*)pReg,4);
	}
	else if (mcu_cmd.header[3] == 0) {
	switch (mcu_cmd.reg_addr)
	{
		case SUB_CHIP:
			PR_DEBUG("set sub chip:val=%d\n",mcu_cmd.reg_value);
			Max_Chip_Num = mcu_cmd.reg_value&0x0f;
			if(Max_Chip_Num==0)
				Max_Chip_Num = 1;
			if(Max_Chip_Num>8)
				Max_Chip_Num = 8;
			break;
		case BTC_RESET:
			PR_DEBUG("btc reset\n");
			if(mcu_cmd.reg_value==0)
			{
				reset_btc_hw();
				reset_trans_buffer(&uart_trans_buffer[COM2]);
				#if defined (USE_STM3210E_EVAL)				
				reset_trans_buffer(&uart_trans_buffer[COM4]);
				#endif
			}
			else if(mcu_cmd.reg_value==1)
			{
				reset_ltc_hw();
				reset_trans_buffer(&uart_trans_buffer[COM3]);
				#if defined (USE_STM3210E_EVAL)
				reset_trans_buffer(&uart_trans_buffer[COM5]);
				#endif
			}
			break;
		case FIFO_RESET:
			PR_DEBUG("fifo reset\n");
			reset_task_fifo(&btc_task_fifo);
			break;
		case USB_RESET:
			PR_DEBUG("usb reset\n");
			break;
		case GCP_RESET:
			PR_DEBUG("gcp reset\n");
			reset_gc3355_chip();
			ltc_com_sel = 0;
			break;
		case GET_FWVER:
			PR_DEBUG("get fw ver\n");
			memcpy((u8 *)(&mcu_cmd.reg_value), fw_ver, 4);
			trans_to_host((u8 *)(&mcu_cmd), REPORT_MAX_LENGTH);
			break;
		case SET_BRATE:
			PR_DEBUG("set bitrate:val=%d\n",mcu_cmd.reg_value);
			//linecoding.bitrate = mcu_cmd->reg_value;
			btc_ltc_uart_config(mcu_cmd.reg_value);
			break;
		case GET_FREE_FIFO:
			PR_DEBUG("get free fifo\n");
			mcu_cmd.reg_value = get_task_fifo_free(&btc_task_fifo);
			trans_to_host((u8 *)(&mcu_cmd), REPORT_MAX_LENGTH);			
			break;
		case EN_DIS_FIFO:
			PR_DEBUG("en dis fifo\n");
			if(mcu_cmd.reg_value==0)
				btc_task_fifo_mode = 0;
			else
				btc_task_fifo_mode = 1;
			reset_task_fifo(&btc_task_fifo);
			break;
		case GET_BTC_INFIFO:
			break;
		default:
			break;
	}
		}
}

static void _dispatch_host_cmd(u8 * cmd, u32 length)
{
	u8 cmd_type;
	//PR_DEBUG("dispatch_host_cmd:%d\n",length);
	//led0_revert();
	if(cmd[0]==HEADER_55 && cmd[1]==HEADER_AA)
	{
		cmd_type = cmd[0+2] & 0xf0;
		switch (cmd_type) {
			case BTC:
				process_btc_cmd(cmd, length);
				break;
			case LTC:
				process_ltc_cmd(cmd, length);
				break;
			case CPM:
				trans_btc_cmd(cmd, length);
				break;
			case BTA:
				break;
			case MCU:
				process_mcu_cmd(cmd, length);
				break;
			default:
				break;
		}
	}	
}

extern volatile u32 FrameCount;
extern  u32 com1_frame_count;
#ifdef USE_USB_AS_HOST_IF
void dispatch_host_cmd(void)
{
	u32 length;
	u8 *p = cmd_buffer;

	if(IS_IRQ_FLAG(USBOUT_IRQ_FLAG)) {
		CLEAR_IRQ_FLAG(USBOUT_IRQ_FLAG);
		FrameCount = 0;
		//PR_DEBUG("usb out int\n");
	}
	/* when usb out int occurs here, it's huge chance to get incomplete data>64 bytes*/
	while (length = get_trans_buffer_cmd(&usb_trans_buffer,p))
	{
		DBG_CMD("get cmd<<",p,length);
		delay_us(CMD_WAIT_TIME);
		_dispatch_host_cmd(p, length);
	}

	if(IS_IRQ_FLAG(USBOUT_IRQ_FLAG))
		return;

	if ((trans_buffer_out_length(&usb_trans_buffer) > 0)) {
		delay_ms(5);
		length = get_trans_buffer(&usb_trans_buffer, p, CMD_MAX_LENGTH);
		_dispatch_host_cmd(p, length);
		//PR_DEBUG("get cmd by delay\n");
	}
}
#else
void dispatch_host_cmd(void)
{
	u32 length;
	u8 *p = cmd_buffer;
	struct TRANS_BUFFER *tb = &uart_trans_buffer[COM1];
	//struct TRANS_BUFFER *tb = &usb_trans_buffer;

	//if(com1_frame_count < 10) {
	//	return;
	//}
	while (length = get_trans_buffer_cmd(tb,p))
	{
		DBG_CMD("get cmd<<",p,length);
		com1_frame_count = 0;
		delay_us(CMD_WAIT_TIME);
		_dispatch_host_cmd(p, length);
	}
	
	if(com1_frame_count < 5) {
		return;
	}
	
	if ((trans_buffer_out_length(tb) > 0)) {
		length = get_trans_buffer(tb, p, CMD_MAX_LENGTH);
		_dispatch_host_cmd(p, length);
		//PR_DEBUG("get cmd by delay\n");
	}
}
#endif
void trans_btc_task(void)
{
	if (IS_IRQ_FLAG(BTC_IRQ_FLAG))
	{
		PR_DEBUG("btc busy\n");
		CLEAR_IRQ_FLAG(BTC_IRQ_FLAG);
	}
}

static void report_nonce_from_uartrx(int com)
{
	int length;
	u32 nonce;
	struct TRANS_BUFFER *tb = &uart_trans_buffer[com];

	while(trans_buffer_out_length(tb) >= 4) {
		CLEAR_IRQ_FLAG((1 << com));
		get_trans_buffer(tb, (u8*)&nonce, 4);
		PR_DEBUG("COM%d",com+1);
		DBG_CMD(">>>",(u8*)&nonce, 4);
#if defined(USE_STM3210E_EVAL)
		if (COM_USART[com] == BTC_COM1 || COM_USART[com] == BTC_COM2)
			report_btc_nonce(nonce);
		else if (COM_USART[com] == LTC_COM1 || COM_USART[com] == LTC_COM2)
			report_ltc_nonce(nonce);
#else
		if (COM_USART[com] == BTC_COM1)
			report_btc_nonce(nonce);
		else if (COM_USART[com] == LTC_COM1)
			report_ltc_nonce(nonce);		
#endif
	}
	if(IS_IRQ_FLAG((1 << com)))
	{
		delay_us(CMD_WAIT_TIME);
		//PR_DEBUG("get btc report data\n");
		CLEAR_IRQ_FLAG((1 << com));		
		length = get_trans_buffer(&uart_trans_buffer[com], (u8*)&nonce, 4);
		flush_trans_buffer(&uart_trans_buffer[com]);
		if (length < 4)/* invalid nonce */ 
			return;
		PR_DEBUG("COM%d",com+1);
		DBG_CMD(">>>",(u8*)&nonce, 4);		
#if defined(USE_STM3210E_EVAL)
		if (COM_USART[com] == BTC_COM1 || COM_USART[com] == BTC_COM2)
			report_btc_nonce(nonce);
		else if (COM_USART[com] == LTC_COM1 || COM_USART[com] == LTC_COM2)
			report_ltc_nonce(nonce);
#else
		if (COM_USART[com] == BTC_COM1)
			report_btc_nonce(nonce);
		else if (COM_USART[com] == LTC_COM1)
			report_ltc_nonce(nonce);		
#endif
		//led0_revert();
	}
}

void report_nonce_to_host(void)
{	
	
	report_nonce_from_uartrx(COM2);
	report_nonce_from_uartrx(COM3);
#if defined(USE_STM3210E_EVAL)	
	report_nonce_from_uartrx(COM4);
	report_nonce_from_uartrx(COM5);
#endif
}

#if 0
void report_nonce_to_host(void)
{
	u32 length;
	u8 * p = report_buffer;
	u32 nonce;
	int i;

//	for (i=0; i<COMn; i++) {
//		length = get_trans_buffer(&uart_trans_buffer[i], p, REPORT_MAX_LENGTH);
//		if (length > 0)
//			trans_buf_to_usb(p, length);
//	}

	if(IS_IRQ_FLAG(COM1_IRQ_FLAG))
	{
		delay_us(200);
		length = get_trans_buffer(&uart_trans_buffer[COM1], p, REPORT_MAX_LENGTH);
		if (length > 0)
			trans_buf_to_usb(p, length);
		CLEAR_IRQ_FLAG(COM1_IRQ_FLAG);
	}
	/*btc channel*/
	if(IS_IRQ_FLAG(COM2_IRQ_FLAG))
	{	
		delay_us(1000);
		//PR_DEBUG("get btc report data\n");
		length = get_trans_buffer(&uart_trans_buffer[COM2], (u8*)&nonce, 4);
		//PR_DEBUG("len=%d,%08X",length, nonce);
		if (length == 4) 
			report_btc_nonce(nonce);
		CLEAR_IRQ_FLAG(COM2_IRQ_FLAG);
		led0_revert();
	}	
	/*ltc channel*/
	if(IS_IRQ_FLAG(COM3_IRQ_FLAG))
	{	
		delay_us(1000);
		//PR_DEBUG("get btc report data\n");
		length = get_trans_buffer(&uart_trans_buffer[COM3], (u8*)&nonce, 4);
		if (length == 4) 
			report_ltc_nonce(nonce);
		CLEAR_IRQ_FLAG(COM3_IRQ_FLAG);
		led0_revert();
	}
#if defined(USE_STM3210E_EVAL)
	/*btc channel*/
	if(IS_IRQ_FLAG(COM4_IRQ_FLAG))
	{	
		delay_us(1000);
		length = get_trans_buffer(&uart_trans_buffer[COM4], (u8*)&nonce, 4);
		if (length == 4) 
			report_btc_nonce(nonce);
		CLEAR_IRQ_FLAG(COM4_IRQ_FLAG);
		led0_revert();
	}
	/*ltc channel*/
	if(IS_IRQ_FLAG(COM5_IRQ_FLAG))
	{		
		delay_us(1000);
		length = get_trans_buffer(&uart_trans_buffer[COM5], (u8*)&nonce, 4);
		if (length == 4) 
			report_ltc_nonce(nonce);
		CLEAR_IRQ_FLAG(COM5_IRQ_FLAG);
		led0_revert();
	}
#endif
}
#endif
