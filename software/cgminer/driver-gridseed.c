/*
 * Copyright 2013 Faster <develop@gridseed.com>
 * Copyright 2012-2013 Andrew Smith
 * Copyright 2012 Luke Dashjr
 * Copyright 2012 Con Kolivas <kernel@kolivas.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#include "config.h"

#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>

#include "miner.h"
#include "usbutils.h"
#include "util.h"

#ifdef WIN32
  #include "compat.h"
  #include <windows.h>
  #include <winsock2.h>
  #include <io.h>
#else
  #include <sys/select.h>
  #include <termios.h>
  #include <sys/stat.h>
  #include <fcntl.h>
#endif /* WIN32 */

#include "elist.h"
#include "miner.h"
#include "usbutils.h"
#include "driver-gridseed.h"
#include "hexdump.c"
#include "util.h"

static const char *gridseed_version = "v3.8.5.20140210.02";

typedef struct s_gridseed_info {
	enum sub_ident	ident;
	// options
	int				baud;
	int				freq;
	int				chips; //chips per module
	int				modules;
	int				usefifo;
	int				btcore;
	// runtime
	struct thr_info	*thr;
	pthread_t		th_read;
	pthread_t		th_send;
	pthread_mutex_t	lock;
	pthread_mutex_t	qlock;
	// state
	int				queued;
	int				dev_queue_len;
	int				soft_queue_len;
	struct work		*workqueue[GRIDSEED_SOFT_QUEUE_LEN];
	int				needworks;  /* how many works need to be queue for device */
	bool			query_qlen; /* true when query device queue length and waiting response */
	cgtimer_t		query_ts;
	int				workdone;
	// LTC proxy
	int				sockltc;
	short			r_port; /* local port to recv request */
	short			s_port; /* remote port to send response */
	struct sockaddr_in toaddr; /* remote address to send response */
} GRIDSEED_INFO;

/* commands to set core frequency */
static const char *opt_frequency[] = {
	"250", "400", "450", "500", "550", "600",
	"650", "700", "750", "800", "850", "900",
	NULL
};

static const char *str_frequency[] = {
	"55AAEF0005002001", "55AA0FFF7A080040",
	"55AAEF000500E001", "55AA0FFF900D0040",
	"55AAEF0005002002", "55AA0FFF420F0040",
	"55AAEF0005006082", "55AA0FFFF4100040",
	"55AAEF000500A082", "55AA0FFFA6120040",
	"55AAEF000500E082", "55AA0FFF58140040",
	"55AAEF0005002083", "55AA0FFF0A160040",
	"55AAEF0005006083", "55AA0FFFBC170040",
	"55AAEF000500A083", "55AA0FFF6E190040",
	"55AAEF000500E083", "55AA0FFF201B0040",
	"55AAEF0005002084", "55AA0FFFD21C0040",
	"55AAEF0005006084", "55AA0FFF841E0040",
	NULL, NULL
};

static const char *str_reset[] = {
	"55AAC000808080800000000001000000", // Chip reset
	"55AAC000E0E0E0E00000000001000000", // FW reset
	NULL
};

static const char *str_init[] = {
	"55AAC000C0C0C0C00500000001000000",
	"55AAEF020000000000000000000000000000000000000000",
	"55AAEF3020000000",
	NULL
};

static const char *str_nofifo[] = {
	"55AAC000D0D0D0D00000000001000000",
	NULL
};

static const char *str_baud[] = {
	"55AAEF2003008014",
	"55AAC000B0B0B0B040420F0001000000",
	"55AA0FFF8A0200C0",
	NULL
};

static const char *str_enable_btc_cores[] = {
	"55AAEF02FFFF000000000000000000000000000000000000",
	"55AAEF02FFFFFFFF00000000000000000000000000000000",
	"55AAEF02FFFFFFFFFFFF0000000000000000000000000000",
	"55AAEF02FFFFFFFFFFFFFFFF000000000000000000000000",
	"55AAEF02FFFFFFFFFFFFFFFFFFFF00000000000000000000",
	"55AAEF02FFFFFFFFFFFFFFFFFFFFFFFF0000000000000000",
	"55AAEF02FFFFFFFFFFFFFFFFFFFFFFFFFFFF000000000000",
	"55AAEF02FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000",
	"55AAEF02FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0000",
	"55AAEF02FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
	NULL
};

static const char *str_ltc_reset[] = {
	"55AA1F2817000000",
	"55AA1F2814000000",
	"55AA1F2817000000",
	NULL
};


static int gc3355_write_data(struct cgpu_info *gridseed, unsigned char *data, int size);
static void gc3355_send_cmds(struct cgpu_info *gridseed, const char *cmds[]);


#ifdef WIN32
static void set_text_color(WORD color)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
	return;
}
#endif

/*
 * check if a UDP port in use? if not, return the socket
 */
static int check_udp_port_in_use(short port)
{
	int sock;
	struct sockaddr_in local;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return -1;

	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0) {
		close(sock);
		return -2;
	}

	return sock;
}

/*
 * search one UDP port to bind
 */
static int gridseed_create_proxy(struct cgpu_info *gridseed, GRIDSEED_INFO *info)
{
	int sock;
	short port;

	sock = info->sockltc;
	if (sock > 0)
		close(sock);
	info->sockltc = -1;

	port = GRIDSEED_PROXY_PORT;
	while(true) {
		sock = check_udp_port_in_use(port);
		if (sock > 0)
			break;
		port++;
	} 
	info->sockltc = sock;
	info->r_port  = port;
	info->s_port  = port + 1000;

	info->toaddr.sin_family = AF_INET;
	info->toaddr.sin_port   = htons(info->s_port);
	info->toaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	applog(LOG_NOTICE, "Create LTC proxy on %d/UDP for %s(%d)", port, gridseed->device_path, gridseed->device_id); 
	return 0;
}

/*
 * send data to cpuminer
 */
static int gridseed_send_ltc_nonce(struct cgpu_info *gridseed, GRIDSEED_INFO *info,
		unsigned char *buf, int buflen)
{
	if (info->sockltc <= 0)
		return 0;

	if (sendto(info->sockltc, buf, buflen, 0, (struct sockaddr *)&(info->toaddr),
			sizeof(info->toaddr)) == buflen)
		return 0;
	else
		return -1;
}

/*
 * check if we have LTC task. timeout 1ms
 */
static void gridseed_recv_ltc(struct cgpu_info *gridseed, GRIDSEED_INFO *info)
{
	fd_set rdfs;
	struct timeval tv;
	unsigned char buf[156];
	int sock;
	int n;

	sock = info->sockltc;
	if (sock <= 0)
		return;

re_read:

	tv.tv_sec = 0;
	tv.tv_usec = 10000;
	FD_ZERO(&rdfs);
	FD_SET(sock, &rdfs);
	if (select(sock+1, &rdfs, NULL, NULL, &tv) <= 0)
		return;

	n = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
	if (n != sizeof(buf))
		goto re_read;

	mutex_lock(&info->qlock);
	if (buf[0] == 0x55 || buf[1] == 0xaa) {
		if (buf[2] == 0x1f) {
			gc3355_send_cmds(gridseed, str_ltc_reset);
			gc3355_write_data(gridseed, buf, sizeof(buf));
		}
		else if (buf[2] == 0x55) { // ping command
			unsigned char nonce[12];
			memcpy(nonce, "\x55\xaa\x55\xaa\x11\x22\x33\x44\x55\x66\x77\x88", 12);
			gridseed_send_ltc_nonce(gridseed, info, nonce, sizeof(nonce));
		}
	}
	mutex_unlock(&info->qlock);
	goto re_read;
}

/*
 * Tell the cpuminer to send new task, after HW init
 */
static void gridseed_request_ltc_task(struct cgpu_info *gridseed, GRIDSEED_INFO *info)
{
	unsigned char nonce[12];
	memcpy(nonce, "\x55\xaa\x55\xaa\x01\x01\x01\x01\x01\x00\x00\x00", 12);
	gridseed_send_ltc_nonce(gridseed, info, nonce, sizeof(nonce));
}

/*---------------------------------------------------------------------------------------*/

static void _transfer(struct cgpu_info *gridseed, uint8_t request_type, uint8_t bRequest,
		uint16_t wValue, uint16_t wIndex, uint32_t *data, int siz, enum usb_cmds cmd)
{
	int err;

	err = usb_transfer_data(gridseed, request_type, bRequest, wValue, wIndex, data, siz, cmd);

	applog(LOG_DEBUG, "%s: cgid %d %s got err %d",
			gridseed->drv->name, gridseed->cgminer_id,
			usb_cmdname(cmd), err);
	return;
}

static int gc3355_write_data(struct cgpu_info *gridseed, unsigned char *data, int size)
{
	int err, wrote;

#if 1
	if (!opt_quiet && opt_debug && data[2] == 0x0f) {
		int i;
#ifndef WIN32
		printf("[1;33m >>> %d : [0m", size);
#else
		set_text_color(FOREGROUND_RED|FOREGROUND_GREEN);
		printf(" >>> %d : ", size);
		set_text_color(FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
#endif
		for(i=0; i<size; i++) {
			printf("%02x", data[i]);
			if (i==3)
				printf(" ");
		}
		printf("\n");
	}
#endif
	err = usb_write(gridseed, data, size, &wrote, C_SENDWORK);
	if (err != LIBUSB_SUCCESS || wrote != size)
		return -1;
	return 0;
}

static int gc3355_get_data(struct cgpu_info *gridseed, unsigned char *buf, int size)
{
	unsigned char *p;
	int readcount;
	int err, amount;

	readcount = size;
	p = buf;
	while(readcount > 0) {
		err = usb_read_once(gridseed, p, readcount, &amount, C_GETRESULTS);
		if (err && err != LIBUSB_ERROR_TIMEOUT)
			return err;
		readcount -= amount;
		p += amount;
	}
#if 1
	if (!opt_quiet && opt_debug && buf[1] != 0xaa) {
		int i;
#ifndef WIN32
		printf("[1;31m <<< %d : [0m", size);
#else
		set_text_color(FOREGROUND_RED);
		printf(" <<< %d : ", size);
		set_text_color(FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
#endif
		for(i=0; i<size; i++) {
			printf("%02x", buf[i]);
			if ((i+1) % 4 == 0)
			printf(" ");
		}
		printf("\n");
	}
#endif
	return 0;
}

static void gc3355_send_cmds(struct cgpu_info *gridseed, const char *cmds[])
{
	unsigned char	ob[512];
	int				i;

	for(i=0; ; i++) {
		if (cmds[i] == NULL)
			break;
		hex2bin(ob, cmds[i], sizeof(ob));
		gc3355_write_data(gridseed, ob, strlen(cmds[i])/2);
		cgsleep_ms(GRIDSEED_COMMAND_DELAY);
	}
	return;
}

static void gc3355_send_cmds_bin(struct cgpu_info *gridseed, const char *cmds[], int size)
{
	int				i;

	for(i=0; ; i++) {
		if (cmds[i] == NULL)
			break;
		gc3355_write_data(gridseed, (char *)cmds[i], size);
		cgsleep_ms(GRIDSEED_COMMAND_DELAY);
	}
	return;
}

static int gc3355_find_freq_index(const char *freq)
{
	int	i, inx=5;

	if (freq != NULL) {
		for(i=0; ;i++) {
			if (opt_frequency[i] == NULL)
				break;
			if (strcmp(freq, opt_frequency[i]) == 0)
				inx = i;
		}
	}
	return inx;
}

static void gc3355_set_core_freq(struct cgpu_info *gridseed)
{
	GRIDSEED_INFO	*info;
	int		inx;
	char	*p;
	char	*cmds[4];

	info = (GRIDSEED_INFO*)(gridseed->device_data);
	inx = info->freq;
	cmds[0] = (char *)str_frequency[inx*2];
	cmds[1] = (char *)str_frequency[inx*2+1];
	cmds[2] = NULL;
	cmds[3] = NULL;
	gc3355_send_cmds(gridseed, (const char **)cmds);
	applog(LOG_INFO, "Set GC3355 core frequency to %sMhz", opt_frequency[inx]);
	return;
}

static void gc3355_enable_btc_cores(struct cgpu_info *gridseed, GRIDSEED_INFO *info)
{
	unsigned char cmd[24], c1, c2;
	uint16_t	mask;
	int i;

	mask = 0x00;
	for(i=0; i<info->btcore; i++)
		mask = mask << 1 | 0x01;

	if (mask == 0)
		return;

	c1 = mask & 0x00ff;
	c2 = mask >> 8;

	memset(cmd, 0, sizeof(cmd));
	memcpy(cmd, "\x55\xAA\xEF\x02", 4);
	for(i=4; i<24; i++) {
		cmd[i] = ((i%2)==0) ? c1 : c2;
		gc3355_write_data(gridseed, cmd, sizeof(cmd));
		cgsleep_ms(GRIDSEED_COMMAND_DELAY);
	}
	return;
}

static void gc3355_set_init_nonce(struct cgpu_info *gridseed)
{
	GRIDSEED_INFO	*info;
	int				i, chips;
	char			**cmds, *p;
	uint32_t		nonce, step;

	info = (GRIDSEED_INFO*)(gridseed->device_data);
	cmds = calloc(sizeof(char*)*(info->chips+1), 1);
	if (unlikely(!cmds))
		quit(1, "Failed to calloc init nonce commands data array");

	step = 0xffffffff / info->chips;
	for(i=0; i<info->chips; i++) {
		p = calloc(8, 1);
		if (unlikely(!p))
			quit(1, "Failed to calloc init nonce commands data");
		memcpy(p, "\x55\xaa\x00\x00", 4);
		p[2] = i;
		nonce = htole32(step*i);
		memcpy(p+4, &nonce, sizeof(nonce));
		cmds[i] = p;
	}
	cmds[i] = NULL;
	gc3355_send_cmds_bin(gridseed, (const char **)cmds, 8);

	for(i=0; i<info->chips; i++)
		free(cmds[i]);
	free(cmds);
	return;
}

static void gc3355_init(struct cgpu_info *gridseed, GRIDSEED_INFO *info, bool set_nonce)
{
	unsigned char buf[512];
	int amount;

	applog(LOG_NOTICE, "System reseting");
	gc3355_send_cmds(gridseed, str_reset);
	cgsleep_ms(200);
	usb_buffer_clear(gridseed);
	usb_read_timeout(gridseed, buf, sizeof(buf), &amount, 10, C_GETRESULTS);
	gc3355_send_cmds(gridseed, str_init);
	gc3355_send_cmds(gridseed, str_ltc_reset);
	gc3355_set_core_freq(gridseed);
	if (set_nonce)
		gc3355_set_init_nonce(gridseed);
	//gc3355_send_cmds(gridseed, str_baud);
	//gc3355_send_cmds(gridseed, str_enable_btc_cores);
	gc3355_enable_btc_cores(gridseed, info);
	if (info->usefifo == 0)
		gc3355_send_cmds(gridseed, str_nofifo);
	gridseed_request_ltc_task(gridseed, info);
	return;
}

static bool get_options(int this_option_offset, struct cgpu_info *gridseed, char *options,
		int *baud, int *freq, int *chips, int *modules, int *usefifo, int *btcore)
{
	unsigned char *ss, *p, *end, *comma, *colon;
	int tmp;

	if (options == NULL)
		return false;

	applog(LOG_NOTICE, "GridSeed options: '%s'", options);
	ss = strdup(options);
	p  = ss;
	end = p + strlen(p);

another:
	comma = strchr(p, ',');
	if (comma != NULL)
		*comma = '\0';
	colon = strchr(p, '=');
	if (colon == NULL)
		goto next;
	*colon = '\0';

	tmp = atoi(colon+1);
	if (strcasecmp(p, "baud")==0) {
		*baud = (tmp != 0) ? tmp : *baud;
	}
	else if (strcasecmp(p, "freq")==0) {
		*freq = gc3355_find_freq_index(colon+1);
	}
	else if (strcasecmp(p, "chips")==0) {
		*chips = (tmp != 0) ? tmp : *chips;
	}
	else if (strcasecmp(p, "modules")==0) {
		*modules = (tmp != 0) ? tmp : *modules;
	}
	else if (strcasecmp(p, "usefifo")==0) {
		*usefifo = tmp;
	}
	else if (strcasecmp(p, "btc")==0) {
		*btcore = tmp;
	}

next:
	if (comma != NULL) {
		p = comma + 1;
		if (p < end)
			goto another;
	}
	free(ss);
	return true;
}

static int gridseed_cp210x_init(struct cgpu_info *gridseed, int interface)
{
	// Enable the UART
	transfer(gridseed, CP210X_TYPE_OUT, CP210X_REQUEST_IFC_ENABLE, CP210X_VALUE_UART_ENABLE,
			interface, C_ENABLE_UART);

	if (gridseed->usbinfo.nodev)
		return -1;

	// Set data control
	transfer(gridseed, CP210X_TYPE_OUT, CP210X_REQUEST_DATA, CP210X_VALUE_DATA,
			interface, C_SETDATA);

	if (gridseed->usbinfo.nodev)
		return -1;

	// Set the baud
	uint32_t data = CP210X_DATA_BAUD;
	_transfer(gridseed, CP210X_TYPE_OUT, CP210X_REQUEST_BAUD, 0,
			interface, &data, sizeof(data), C_SETBAUD);

	return 0;
}

static int gridseed_ftdi_init(struct cgpu_info *gridseed, int interface)
{
	int err;

	// Reset
	err = usb_transfer(gridseed, FTDI_TYPE_OUT, FTDI_REQUEST_RESET,
				FTDI_VALUE_RESET, interface, C_RESET);

	applog(LOG_DEBUG, "%s%i: reset got err %d",
		gridseed->drv->name, gridseed->device_id, err);

	if (gridseed->usbinfo.nodev)
		return -1;

	// Set latency
	err = usb_transfer(gridseed, FTDI_TYPE_OUT, FTDI_REQUEST_LATENCY,
			   GRIDSEED_LATENCY, interface, C_LATENCY);

	applog(LOG_DEBUG, "%s%i: latency got err %d",
		gridseed->drv->name, gridseed->device_id, err);

	if (gridseed->usbinfo.nodev)
		return -1;

	// Set data
	err = usb_transfer(gridseed, FTDI_TYPE_OUT, FTDI_REQUEST_DATA,
				FTDI_VALUE_DATA_AVA, interface, C_SETDATA);

	applog(LOG_DEBUG, "%s%i: data got err %d",
		gridseed->drv->name, gridseed->device_id, err);

	if (gridseed->usbinfo.nodev)
		return -1;

	// Set the baud
	err = usb_transfer(gridseed, FTDI_TYPE_OUT, FTDI_REQUEST_BAUD, FTDI_VALUE_BAUD_AVA,
				(FTDI_INDEX_BAUD_AVA & 0xff00) | interface,
				C_SETBAUD);

	applog(LOG_DEBUG, "%s%i: setbaud got err %d",
		gridseed->drv->name, gridseed->device_id, err);

	if (gridseed->usbinfo.nodev)
		return -1;

	// Set Modem Control
	err = usb_transfer(gridseed, FTDI_TYPE_OUT, FTDI_REQUEST_MODEM,
				FTDI_VALUE_MODEM, interface, C_SETMODEM);

	applog(LOG_DEBUG, "%s%i: setmodemctrl got err %d",
		gridseed->drv->name, gridseed->device_id, err);

	if (gridseed->usbinfo.nodev)
		return -1;

	// Set Flow Control
	err = usb_transfer(gridseed, FTDI_TYPE_OUT, FTDI_REQUEST_FLOW,
				FTDI_VALUE_FLOW, interface, C_SETFLOW);

	applog(LOG_DEBUG, "%s%i: setflowctrl got err %d",
		gridseed->drv->name, gridseed->device_id, err);

	if (gridseed->usbinfo.nodev)
		return -1;

	/* Avalon repeats the following */
	// Set Modem Control
	err = usb_transfer(gridseed, FTDI_TYPE_OUT, FTDI_REQUEST_MODEM,
				FTDI_VALUE_MODEM, interface, C_SETMODEM);

	applog(LOG_DEBUG, "%s%i: setmodemctrl 2 got err %d",
		gridseed->drv->name, gridseed->device_id, err);

	if (gridseed->usbinfo.nodev)
		return -1;

	// Set Flow Control
	err = usb_transfer(gridseed, FTDI_TYPE_OUT, FTDI_REQUEST_FLOW,
				FTDI_VALUE_FLOW, interface, C_SETFLOW);

	applog(LOG_DEBUG, "%s%i: setflowctrl 2 got err %d",
		gridseed->drv->name, gridseed->device_id, err);

	if (gridseed->usbinfo.nodev)
		return -1;

	return 0;
}

static int gridseed_pl2303_init(struct cgpu_info *gridseed, int interface)
{
	// Set Data Control
	transfer(gridseed, PL2303_CTRL_OUT, PL2303_REQUEST_CTRL, PL2303_VALUE_CTRL,
			 interface, C_SETDATA);

	if (gridseed->usbinfo.nodev)
		return;

	// Set Line Control
	uint32_t ica_data[2] = { PL2303_VALUE_LINE0, PL2303_VALUE_LINE1 };
	_transfer(gridseed, PL2303_CTRL_OUT, PL2303_REQUEST_LINE, PL2303_VALUE_LINE,
			 interface, &ica_data[0], PL2303_VALUE_LINE_SIZE, C_SETLINE);

	if (gridseed->usbinfo.nodev)
		return;

	// Vendor
	transfer(gridseed, PL2303_VENDOR_OUT, PL2303_REQUEST_VENDOR, PL2303_VALUE_VENDOR,
			 interface, C_VENDOR);

	return;
}

static void gridseed_initialise(struct cgpu_info *gridseed, GRIDSEED_INFO *info)
{
	int err, interface;
	enum sub_ident ident;

	if (gridseed->usbinfo.nodev)
		return;

	interface = usb_interface(gridseed);
	ident = usb_ident(gridseed);

	switch(ident) {
		case IDENT_GSD:
			err = 0;
			break;
		case IDENT_GSD1:
			err = gridseed_cp210x_init(gridseed, interface);
			break;
		case IDENT_GSD2:
			err = gridseed_ftdi_init(gridseed, interface);
			break;
		case IDENT_GSD3:
			err = gridseed_pl2303_init(gridseed, interface);
			break;
		default:
			quit(1, "gridseed_intialise() called with invalid %s cgid %i ident=%d",
				gridseed->drv->name, gridseed->cgminer_id, ident);
	}
	if (err)
		return;

	gc3355_init(gridseed, info, true);

	return;
}

static struct cgpu_info *gridseed_detect_one(libusb_device *dev, struct usb_find_devices *found)
{
	struct cgpu_info *gridseed;
	GRIDSEED_INFO *info;
	int this_option_offset;
	int baud, freq, chips, modules, usefifo, btcore;
	int err, wrote;
	bool configured;
	unsigned char rbuf[GRIDSEED_READ_SIZE];
	unsigned char *p;
#if 0
	const char detect_cmd[] =
		"55aa0f01"
		"4a548fe471fa3a9a1371144556c3f64d"
		"2500b4826008fe4bbf7698c94eba7946"
		"ce22a72f4f6726141a0b3287eeeeeeee";
	unsigned char detect_data[52];
#else
	const char detect_cmd[] = "55aac000909090900000000001000000";
	unsigned char detect_data[16];
#endif

	gridseed = usb_alloc_cgpu(&gridseed_drv, GRIDSEED_MINER_THREADS);
	if (!usb_init(gridseed, dev, found))
		goto shin;

	libusb_reset_device(gridseed->usbdev->handle);

	baud = GRIDSEED_DEFAULT_BAUD;
	chips = GRIDSEED_DEFAULT_CHIPS;
	modules = GRIDSEED_DEFAULT_MODULES;
	freq = gc3355_find_freq_index(GRIDSEED_DEFAULT_FREQUENCY);
	usefifo = GRIDSEED_DEFAULT_USEFIFO;
	btcore = GRIDSEED_DEFAULT_BTCORE;

	this_option_offset = 0;
	configured = get_options(this_option_offset, gridseed, opt_gridseed_options,
		&baud, &freq, &chips, &modules, &usefifo, &btcore);

	info = (GRIDSEED_INFO*)calloc(sizeof(GRIDSEED_INFO), 1);
	if (unlikely(!info))
		quit(1, "Failed to calloc gridseed_info data");
	gridseed->device_data = (void *)info;

	update_usb_stats(gridseed);

	//if (configured) {
		info->baud = baud;
		info->freq = freq;
		info->chips = chips;
		info->modules = modules;
		info->usefifo = usefifo;
		info->btcore = btcore;
	//}

	gridseed->usbdev->usb_type = USB_TYPE_STD;
	gridseed_initialise(gridseed, info);

	/* send testing work to chips */
	hex2bin(detect_data, detect_cmd, sizeof(detect_data));
	if (gc3355_write_data(gridseed, detect_data, sizeof(detect_data))) {
		applog(LOG_DEBUG, "Failed to write work data to %i, err %d",
			gridseed->device_id, err);
		goto unshin;
	}

	/* waiting for return */
	if (gc3355_get_data(gridseed, rbuf, GRIDSEED_READ_SIZE)) {
		applog(LOG_DEBUG, "No response from %i",
			gridseed->device_id);
		goto unshin;
	}

	if (memcmp(rbuf, "\x55\xaa\xc0\x00\x90\x90\x90\x90", GRIDSEED_READ_SIZE-4) != 0) {
		applog(LOG_DEBUG, "Bad response from %i",
			gridseed->device_id);
		goto unshin;
	}

	p = bin2hex(rbuf+GRIDSEED_READ_SIZE-4, 4);
	applog(LOG_NOTICE, "Device found, firmware version 0x%s, driver version %s", p, gridseed_version);
	free(p);

	if (!add_cgpu(gridseed))
		goto unshin;

	return gridseed;

unshin:
	usb_uninit(gridseed);
	free(gridseed->device_data);
	gridseed->device_data = NULL;

shin:
	gridseed = usb_free_cgpu(gridseed);
	return NULL;
}

static bool gridseed_send_query_cmd(struct cgpu_info *gridseed, GRIDSEED_INFO *info)
{
	unsigned char *cmd = "\x55\xaa\xc0\x00\xa0\xa0\xa0\xa0\x00\x00\x00\x00\x01\x00\x00\x00";
	cgtimer_t ts_now, ts_res;
	bool ret = false;

	cgtimer_time(&ts_now);
	mutex_lock(&info->qlock);
	if (!info->query_qlen) {
		cgtimer_sub(&ts_now, &info->query_ts, &ts_res);
#ifndef WIN32
		if (ts_res.tv_sec > 0) {
#else
		if (ts_res.QuadPart > 10000000) {
#endif
			if (gc3355_write_data(gridseed, cmd, 16) == 0) {
				info->query_qlen = true;
				ret = true;
			}
		}
	}
	mutex_unlock(&info->qlock);
	return ret;
}

static bool gridseed_send_task(struct cgpu_info *gridseed, GRIDSEED_INFO *info,
			struct work *work)
{
	unsigned char cmd[52];

	memcpy(cmd, "\x55\xaa\x0f\x01", 4);
	memcpy(cmd+4, work->midstate, 32);
	memcpy(cmd+36, work->data+64, 12);
	memcpy(cmd+48, &(work->id), 4);
	return (gc3355_write_data(gridseed, cmd, sizeof(cmd)) == 0);
}

static void gridseed_get_queue_length(struct cgpu_info *gridseed, GRIDSEED_INFO *info,
		unsigned char *data)
{
	uint32_t qlen;

	memcpy(&qlen, data+8, 4);
	qlen = htole32(qlen);

	mutex_lock(&info->qlock);
	info->query_qlen = false;
	info->dev_queue_len = GRIDSEED_MCU_QUEUE_LEN - qlen;
	info->needworks = qlen;
	cgtimer_time(&info->query_ts);
	mutex_unlock(&info->qlock);
	return;
}

static void gridseed_parse_mcu_command(struct cgpu_info *gridseed, GRIDSEED_INFO *info,
		unsigned char *data)
{
	if (memcmp(data+4, "\xa0\xa0\xa0\xa0", 4) == 0) {
		/* FIFO idle slots */
		gridseed_get_queue_length(gridseed, info, data);
	} else if (memcmp(data+4, "\x4d\x43\x55\x52\x45\x53\x45\x54", 8) == 0) {
		/* MCU watchdog did HW reset. Re-init GC3355 chips */
		gc3355_init(gridseed, info, true);
	}
}

static void __gridseed_purge_work_queue(struct cgpu_info *gridseed, GRIDSEED_INFO *info, int newstart)
{
	int i;

	if (newstart <= 0 || newstart >= info->soft_queue_len)
		return;

	for(i=0; i<newstart; i++) {
		work_completed(gridseed, info->workqueue[i]);
		info->workdone++;
	}
	memmove(&(info->workqueue[0]), &(info->workqueue[newstart]),
			sizeof(struct work*)*(info->soft_queue_len - newstart));
	info->soft_queue_len -= newstart;
	return;
}

static void gridseed_purge_work_queue(struct cgpu_info *gridseed, GRIDSEED_INFO *info, int newstart)
{
	mutex_lock(&info->qlock);
	__gridseed_purge_work_queue(gridseed, info, newstart);
	mutex_unlock(&info->qlock);
}

static void gridseed_test_btc_nonce(struct cgpu_info *gridseed, GRIDSEED_INFO *info,
		struct thr_info *thr, unsigned char *data)
{
	struct work *work;
	uint32_t nonce;
	int workid, index, i;
	bool valid = false;
	bool nowork = false;

	memcpy(&workid, data+8, 4);
	memcpy(&nonce, data+4, 4);
	nonce = htole32(nonce);

	mutex_lock(&info->qlock);
	nowork = (info->soft_queue_len <= 0);
	for(i=0; i<info->soft_queue_len; i++) {
		struct work *dupwork;
		work = info->workqueue[i];
		if (work->devflag == false)
			continue;
		if (work->id > workid)
			break;
		dupwork = copy_work(work);
		if (dupwork == NULL)
			continue;
		if (test_nonce(dupwork, nonce)) {
			submit_tested_work(thr, dupwork);
			index = i;
			valid = true;
			free_work(dupwork);
			break;
		} else
			free_work(dupwork);
	}
	if (valid)
		__gridseed_purge_work_queue(gridseed, info, index);
	mutex_unlock(&info->qlock);

	if (!valid && !nowork)
		inc_hw_errors(thr);
	return;
}

static void gridseed_parse_results(struct cgpu_info *gridseed, GRIDSEED_INFO *info,
		struct thr_info *thr, unsigned char *readbuf, int *offset)
{
	unsigned char *p;
	int size;

	p = readbuf;
	size = *offset;

one_cmd:
	/* search the staring 0x55 */
	while(size > 0) {
		if (likely(*p == 0x55))
			break;
		p++;
		size--;
	}
	if (size < GRIDSEED_READ_SIZE)
		goto out_cmd;

	switch(p[1]) {
		case 0xaa:
			/* Queue length result */
			gridseed_parse_mcu_command(gridseed, info, p);
			break;
		case 0x10:
			/* BTC result */
			gridseed_test_btc_nonce(gridseed, info, thr, p);
			break;
		case 0x20:
			/* LTC result */
			gridseed_send_ltc_nonce(gridseed, info, p, GRIDSEED_READ_SIZE);
			break;
	}

	p += GRIDSEED_READ_SIZE;
	size -= GRIDSEED_READ_SIZE;
	goto one_cmd;

out_cmd:
	if (size > 0)
		memmove(readbuf, p, size);
	*offset = size;
	return;
}

static bool gridseed_check_new_task(struct cgpu_info *gridseed, GRIDSEED_INFO *info)
{
	cgtimer_t ts_now, ts_res;
	bool ret = false;

	cgtimer_time(&ts_now);
	mutex_lock(&info->qlock);
	cgtimer_sub(&ts_now, &info->query_ts, &ts_res);
#ifndef WIN32
	if (ts_res.tv_sec > 0 || ts_res.tv_nsec > 350000000) {
#else
	if (ts_res.QuadPart > 3500000) {
#endif
		info->query_qlen = false;
		info->dev_queue_len = 1;
		info->needworks = 1;
		cgtimer_time(&info->query_ts);
	}
	mutex_unlock(&info->qlock);
}

/*
 * Thread to read response from Miner device
 */
static void *gridseed_get_results(void *userdata)
{
	struct cgpu_info *gridseed = (struct cgpu_info *)userdata;
	GRIDSEED_INFO *info = gridseed->device_data;
	struct thr_info *thr = info->thr;
	char threadname[24];
	unsigned char readbuf[GRIDSEED_READBUF_SIZE];
	int offset = 0, ret;

	snprintf(threadname, sizeof(threadname), "GridSeed_Recv/%d", gridseed->device_id);
	RenameThread(threadname);
	applog(LOG_NOTICE, "GridSeed: recv thread running, %s", threadname);

	while(likely(!gridseed->shutdown)) {
		unsigned char buf[GRIDSEED_READ_SIZE];

		if (offset >= GRIDSEED_READ_SIZE)
			gridseed_parse_results(gridseed, info, thr, readbuf, &offset);

		if (unlikely(offset + GRIDSEED_READ_SIZE >= GRIDSEED_READBUF_SIZE)) {
			applog(LOG_ERR, "Read buffer overflow, resetting %d", gridseed->device_id);
			offset = 0;
		}

		ret = gc3355_get_data(gridseed, buf, sizeof(buf));
		if (ret == LIBUSB_ERROR_NO_DEVICE)
			gridseed->shutdown = true;
		if (unlikely(ret != 0))
			continue;

		if (opt_debug) {
			applog(LOG_DEBUG, "GridSeed: get %d bytes", GRIDSEED_READ_SIZE);
			hexdump((uint8_t *)buf, GRIDSEED_READ_SIZE);
		}

		memcpy(readbuf + offset, buf, GRIDSEED_READ_SIZE);
		offset += GRIDSEED_READ_SIZE;
	}
	return NULL;
}

/*
 * Thread to send task and queue length query command to device
 */
static void *gridseed_send_command(void *userdata)
{
	struct cgpu_info *gridseed = (struct cgpu_info *)userdata;
	GRIDSEED_INFO *info = gridseed->device_data;
	char threadname[24];
	int i;

	snprintf(threadname, sizeof(threadname), "GridSeed_Send/%d", gridseed->device_id);
	RenameThread(threadname);
	applog(LOG_NOTICE, "GridSeed: send thread running, %s", threadname);

	while(likely(!gridseed->shutdown)) {
		cgsleep_ms(10);
		if (info->usefifo == 0) {
			/* mark the first work in queue as complete after several ms */
			if (gridseed_check_new_task(gridseed, info))
				continue;
		} else {
			/* send query command to device */
			if (gridseed_send_query_cmd(gridseed, info))
				continue;
		}
		/* send task to device */
		mutex_lock(&info->qlock);
		for(i=0; i<info->soft_queue_len; i++) {
			if (info->workqueue[i] && info->workqueue[i]->devflag == false) {
				if (gridseed_send_task(gridseed, info, info->workqueue[i])) {
					info->workqueue[i]->devflag = true;
					break;
				}
			}
		}
		mutex_unlock(&info->qlock);
		/* recv LTC task and send to device */
		gridseed_recv_ltc(gridseed, info);
	}
	return NULL;
}

/*========== functions for struct device_drv ===========*/

static void gridseed_detect(bool __maybe_unused hotplug)
{
	usb_detect(&gridseed_drv, gridseed_detect_one);
}

static bool gridseed_prepare(struct thr_info *thr)
{
	struct cgpu_info *gridseed = thr->cgpu;
	GRIDSEED_INFO *info = gridseed->device_data;

	info->thr = thr;
	mutex_init(&info->lock);
	mutex_init(&info->qlock);

	info->queued = 0;
	info->dev_queue_len = GRIDSEED_MCU_QUEUE_LEN;
	info->soft_queue_len = 0;
	info->query_qlen = false;
	info->needworks = 0;
	memset(&info->workqueue, 0, sizeof(struct work *)*GRIDSEED_SOFT_QUEUE_LEN);
	cgtimer_time(&info->query_ts);
	info->workdone = 0;

	if (info->sockltc > 0)
		close(info->sockltc);
	info->sockltc = -1;

	if (pthread_create(&info->th_read, NULL, gridseed_get_results, (void*)gridseed))
		quit(1, "Failed to create GridSeed read thread");

	if (pthread_create(&info->th_send, NULL, gridseed_send_command, (void*)gridseed))
		quit(1, "Failed to create GridSeed send thread");

	gridseed_create_proxy(gridseed, info);
	gridseed_request_ltc_task(gridseed, info);

	applog(LOG_NOTICE, "GridSeed device opened on %s", gridseed->device_path);
	return true;
}

static bool gridseed_full(struct cgpu_info *gridseed)
{
	GRIDSEED_INFO *info = gridseed->device_data;
	struct work *work;
	int subid, slot;
	bool ret = true;

	//applog(LOG_NOTICE, "[1;32mEntering[0m %s", __FUNCTION__);
	mutex_lock(&info->qlock);
	if (info->needworks <= 0)
		goto out_unlock;

	work = get_queued(gridseed);
	if (unlikely(!work)) {
		ret = false;
		goto out_unlock;
	}
	subid = info->queued++;
	work->subid = subid;
	work->devflag = false; /* true when send to device */

	if (info->soft_queue_len >= GRIDSEED_SOFT_QUEUE_LEN)
		__gridseed_purge_work_queue(gridseed, info, 1);
	info->workqueue[info->soft_queue_len++] = work;
	info->needworks--;

	ret = (info->needworks <= 0);

out_unlock:
	mutex_unlock(&info->qlock);
	return ret;
}

static int64_t gridseed_scanhash(struct thr_info *thr)
{
	struct cgpu_info *gridseed = thr->cgpu;
	GRIDSEED_INFO *info = gridseed->device_data;
	int64_t hashs;

	mutex_lock(&info->qlock);
	hashs = info->workdone * 0xffffffffL;
	info->workdone = 0;
	mutex_unlock(&info->qlock);
	return hashs;
}

static void gridseed_flush_work(struct cgpu_info *gridseed)
{
#if 0
	GRIDSEED_INFO *info = gridseed->device_data;
	int i;

	applog(LOG_NOTICE, "Work updated, flushing work queue %d", gridseed->device_id);
	mutex_lock(&info->qlock);
	for(i=0; i<info->soft_queue_len; i++) {
		work_completed(gridseed, info->workqueue[i]);
	}
	info->soft_queue_len = 0;
	mutex_unlock(&info->qlock);
#endif
	return;
}

static struct api_data *gridseed_api_stats(struct cgpu_info *cgpu)
{
	applog(LOG_NOTICE, "[1;32mEntering[0m %s", __FUNCTION__);
	return NULL;
}

static void get_gridseed_statline_before(char *buf, size_t bufsiz, struct cgpu_info *gridseed)
{
	//applog(LOG_NOTICE, "[1;32mEntering[0m %s", __FUNCTION__);
	return;
}

static char *gridseed_set_device(struct cgpu_info *gridseed, char *option, char *setting, char *replybuf)
{
	applog(LOG_NOTICE, "[1;32mEntering[0m %s", __FUNCTION__);
	return NULL;
}

static void gridseed_init(struct cgpu_info *gridseed)
{
	applog(LOG_NOTICE, "[1;32mEntering[0m %s", __FUNCTION__);
	return;
}

static void gridseed_shutdown(struct thr_info *thr)
{
	struct cgpu_info *gridseed = thr->cgpu;
	GRIDSEED_INFO *info = gridseed->device_data;

	applog(LOG_NOTICE, "Device %d is going to shutdown", gridseed->device_id);
	pthread_join(info->th_send, NULL);
	pthread_join(info->th_read, NULL);
	mutex_destroy(&info->qlock);
	mutex_destroy(&info->lock);
	if (info->sockltc > 0) {
		close(info->sockltc);
		info->sockltc = -1;
	}
	/** TODO do we need to release work in queue? */
	return;
}

static void gridseed_hw_errors(struct thr_info *thr)
{
	struct cgpu_info *gridseed = thr->cgpu;
	GRIDSEED_INFO *info = gridseed->device_data;

	if (gridseed->hw_errors > 5) {
		gc3355_init(gridseed, info, true);
		gridseed->hw_errors = 0;
		applog(LOG_ERR, "HW error, do hardware reset");
	}
	return;
}

/* driver functions */
struct device_drv gridseed_drv = {
	.drv_id = DRIVER_gridseed,
	.dname = "gridseed",
	.name = "GSD",
	.drv_detect = gridseed_detect,
	.thread_prepare = gridseed_prepare,
	.hash_work = hash_queued_work,
	.queue_full = gridseed_full,
	.scanwork = gridseed_scanhash,
	.flush_work = gridseed_flush_work,
	.get_api_stats = gridseed_api_stats,
	.get_statline_before = get_gridseed_statline_before,
	.set_device = gridseed_set_device,
	.reinit_device = gridseed_init,
	.thread_shutdown = gridseed_shutdown,
	.hw_error = gridseed_hw_errors,
};
