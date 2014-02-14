#ifndef __MINER_H_
#define __MINER_H_
/* miner.h
*/
#define USE_USB_AS_HOST_IF
//#define USE_UART1_AS_HOST_IF
 #define  FW_VER_HH 0x01
 #define  FW_VER_HL 0x14
 #define  FW_VER_LH 0x01
 #define  FW_VER_LL 0x13
 
#define DISPATCH_CMD_FRAME_INTERVAL 6
#define CMD_WAIT_TIME 1200 /* us */
#define BTC_COM1 EVAL_COM2
#define BTC_COM2 EVAL_COM4
#define LTC_COM1 EVAL_COM3
#define LTC_COM2 EVAL_COM5

#define GC3355_NUM 0x05
#define HEADER_55 0x55
#define HEADER_AA 0xaa
#define BTC_TASK_FIFO_LEN    0x08
#define CMD_MAX_LENGTH 256
#define REPORT_MAX_LENGTH 12

#define 	SUB_CHIP  0xC0C0C0C0
#define 	BTC_RESET  0xE0E0E0E0
#define 	FIFO_RESET  0xf0f0f0f0
#define 	USB_RESET  0x10101010
#define 	GCP_RESET  0x80808080
#define 	GET_FWVER  0x90909090
 #define  FW_VER_HH 0x01
 #define  FW_VER_HL 0x14
 #define  FW_VER_LH 0x01
 #define  FW_VER_LL 0x13
#define 	SET_BRATE  0xB0B0B0B0
#define 	GET_FREE_FIFO  0xA0A0A0A0
#define 	EN_DIS_FIFO  0xD0D0D0D0
#define 	GET_BTC_INFIFO  0x70707070

enum CMD_TYPE
{
	BTC = 0x00,
	LTC = 0x10,
	CPM = 0xe0,
	BTA = 0xf0,
	MCU = 0xc0,
	ERR = 0xff
};

typedef struct _MINER_CMD_ {
	u8 *cmd;
	u32 length;
	u32 cmd_type;
}MINER_CMD;

typedef struct _BTC_TASK_CMD_{
	u8 header[4];
	u32 nonce;
	u8  data[40];
	u32 taskid;
}BTC_TASK_CMD;

typedef struct _LTC_TASK_CMD_{
	u8 header[4];
	u8  data[140];
	u32 nonce_min;
	u32 nonce_max;
	u32 taskid;
}LTC_TASK_CMD;

typedef struct _TASK_NONCE_ {
	u8 header[4];
	u32 nonce;
	u32 taskid;
}TASK_NONCE;

typedef struct _LTC_NONCE_INIT_ {
	u8 header[4];
	u32 nonce_min;
	u32 nonce_max;
}LTC_NONCE_INIT;

typedef struct _GENENAL_CMD_{
	u8 header[4];
	u8 data[156];
	u32 length;
}GENENAL_CMD;

typedef struct _MCU_CMD_{
	u8 header[4];
	u32 reg_addr;
	u32 reg_value;
	u32 reg_len;
}MCU_CMD;

typedef struct _TASK_FIFO_ {
	BTC_TASK_CMD task[BTC_TASK_FIFO_LEN + 1];
	u32 size;
	u32 in;
	u32 out;
	u32 full;
}TASK_FIFO;

void dispatch_host_cmd(void);
void report_nonce_to_host(void);
#endif /*__MINER_H_*/
