/*
 * Driver for GC3355 chip to mine Litecoin, power by GridChip & GridSeed
 *
 * Copyright 2013 faster <develop@gridseed.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#ifndef WIN32
#include <termios.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#else
#include <windows.h>
#include <winsock2.h>
#include <io.h>
typedef unsigned int speed_t;
#define  B115200  115200
#endif
#include <ctype.h>

#define GC3355_MINER_VERSION	"v2.3.2.20140115.01"
#define GC3355_VERSION			"GC3355-USB-DUAL-" GC3355_MINER_VERSION

static const char *gc3355_version = GC3355_MINER_VERSION;

#define GRIDSEED_LTC_PROXY_PORT	4350

struct gc3355_dev {
	int		id;
	int		dev_fd;
	// LTC proxy
	int		sockltc;
	short	r_port; /* local port to recv request */
	short	s_port; /* remote port to send response */
	struct sockaddr_in	toaddr; /* remote address to send response */
	// Runtime
	bool	resend;
};

/* commands to set core frequency */
static const char *str_frequency[] = {
	"250", "400", "450", "500", "550", "600",
	"650", "675", "700", "750", "800", "850", "900",
	NULL
};
static const char *cmd_frequency[] = {
	"55AAEF0005002001",
	"55AAEF000500E001",
	"55AAEF0005002002",
	"55AAEF0005006082",
	"55AAEF000500A082",
	"55AAEF000500E082",
	"55AAEF0005002083",
	"55AAEF000500A096",
	"55AAEF0005006083",
	"55AAEF000500A083",
	"55AAEF000500E083",
	"55AAEF0005002084",
	"55AAEF0005006084",
	NULL
};

static const char *cmd_enablecors[] = {
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

/* commands for single LTC mode */
static const char *single_cmd_init[] = {
	"55aac000808080800000000001000000",
	"55AAC000C0C0C0C00500000001000000",
	"55AAEF020000000000000000000000000000000000000000",
	"55AAEF3020000000",
	//"55AA1F2814000000",
	"55AA1F2817000000",
	NULL
};

static const char *single_cmd_reset[] = {
	"55AA1F2816000000",
	"55AA1F2817000000",
	NULL
};

static const char *cmd_chipbaud[] = {
	"55AAC000B0B0B0B040420F0001000000",
	NULL
}; 

/* external functions */
extern void scrypt_1024_1_1_256(const uint32_t *input, uint32_t *output,
    uint32_t *midstate, unsigned char *scratchpad);

/* local functions */
static int gc3355_scanhash(struct gc3355_dev *gc3355, uint32_t *pdata, unsigned char *scratchbuf,
		const uint32_t *ptarget, bool keep_work);

/* print in hex format, for debuging */
static void print_hex(unsigned char *data, int len)
{
    int             i, j, s, blank;
    unsigned char   *p=data;

    for(i=s=0; i<len; i++,p++) {
        if ((i%16)==0) {
            s = i;
            printf("%04x :", i);
        }
        printf(" %02x", *p);
        if (((i%16)==7) && (i!=(len-1)))
            printf(" -");
        else if ((i%16)==15) {
            printf("    ");
            for(j=s; j<=i; j++) {
                if (isprint(data[j]))
                    printf("%c", data[j]);
                else
                    printf(".");
            }
            printf("\n");
        }
    }
    if ((i%16)!=0) {
        blank = ((16-i%16)*3+4) + (((i%16)<=8) ? 2 : 0);
        for(j=0; j<blank; j++)
            printf(" ");
        for(j=s; j<i; j++) {
            if (isprint(data[j]))
                printf("%c", data[j]);
            else
                printf(".");
        }
        printf("\n");
    }
}

#ifdef WIN32
static void set_text_color(WORD color)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
	return;
}
#endif

/* close UART device */
static void gc3355_close(int fd)
{
	if (fd > 0)
		close(fd);
	return;
}

/* open UART device */
static int gc3355_open(struct gc3355_dev *gc3355, const char *devname, speed_t baud)
{
#ifdef WIN32
	DWORD	timeout = 1;

	if (gc3355->dev_fd > 0)
		gc3355_close(gc3355->dev_fd);

	HANDLE hSerial = CreateFile(devname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (unlikely(hSerial == INVALID_HANDLE_VALUE))
	{
		DWORD e = GetLastError();
		switch (e) {
		case ERROR_ACCESS_DENIED:
			applog(LOG_ERR, "%d: Do not have user privileges required to open %s", gc3355->id, devname);
			break;
		case ERROR_SHARING_VIOLATION:
			applog(LOG_ERR, "%d: %s is already in use by another process", gc3355->id, devname);
			break;
		default:
			applog(LOG_DEBUG, "%d: Open %s failed, GetLastError:%u", gc3355->id, devname, e);
			break;
		}
		return -1;
	}

	// thanks to af_newbie for pointers about this
	COMMCONFIG comCfg = {0};
	comCfg.dwSize = sizeof(COMMCONFIG);
	comCfg.wVersion = 1;
	comCfg.dcb.DCBlength = sizeof(DCB);
	comCfg.dcb.BaudRate = baud;
	comCfg.dcb.fBinary = 1;
	comCfg.dcb.fDtrControl = DTR_CONTROL_ENABLE;
	comCfg.dcb.fRtsControl = RTS_CONTROL_ENABLE;
	comCfg.dcb.ByteSize = 8;

	SetCommConfig(hSerial, &comCfg, sizeof(comCfg));

	// Code must specify a valid timeout value (0 means don't timeout)
	const DWORD ctoms = (timeout * 100);
	COMMTIMEOUTS cto = {ctoms, 0, ctoms, 0, ctoms};
	SetCommTimeouts(hSerial, &cto);

	PurgeComm(hSerial, PURGE_RXABORT);
	PurgeComm(hSerial, PURGE_TXABORT);
	PurgeComm(hSerial, PURGE_RXCLEAR);
	PurgeComm(hSerial, PURGE_TXCLEAR);

	gc3355->dev_fd = _open_osfhandle((intptr_t)hSerial, 0);
	if (gc3355->dev_fd < 0)
		return -1;
	return 0;

#else

	struct termios	my_termios;
	int fd;

	if (gc3355->dev_fd > 0)
		gc3355_close(gc3355->dev_fd);

    fd = open(devname, O_RDWR | O_CLOEXEC | O_NOCTTY);
	if (fd < 0) {
		if (errno == EACCES)
			applog(LOG_ERR, "%d: Do not have user privileges to open %s", gc3355->id, devname);
		else
			applog(LOG_ERR, "%d: failed open device %s", gc3355->id, devname);
		return 1;
	}

	tcgetattr(fd, &my_termios);
	cfsetispeed(&my_termios, baud);
	cfsetospeed(&my_termios, baud);
	cfsetspeed(&my_termios,  baud);

	my_termios.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
	my_termios.c_cflag |= CS8;
	my_termios.c_cflag |= CREAD;
	my_termios.c_cflag |= CLOCAL;

	my_termios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK |
			ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	my_termios.c_oflag &= ~OPOST;
	my_termios.c_lflag &= ~(ECHO | ECHOE | ECHONL | ICANON | ISIG | IEXTEN);

	// Code must specify a valid timeout value (0 means don't timeout)
	my_termios.c_cc[VTIME] = (cc_t)10;
	my_termios.c_cc[VMIN] = 0;

	tcsetattr(fd, TCSANOW, &my_termios);
	tcflush(fd, TCIOFLUSH);
	gc3355->dev_fd = fd;

	return 0;

#endif
}

/* send data to UART */
static int gc3355_write(struct gc3355_dev *gc3355, const void *buf, size_t buflen)
{
	size_t ret;
	int i;
	unsigned char *p;

	if (!opt_quiet) {
		//printf("^[[1;33mSend to UART: %u^[[0m\n", bufLen);
		//print_hex((unsigned char *)buf, bufLen);
		p = (unsigned char *)buf;
#ifndef WIN32
		printf("[1;33m%d: >>> LTC :[0m ", gc3355->id);
#else
		set_text_color(FOREGROUND_RED | FOREGROUND_GREEN);
		printf("%d: >>> LTC : ", gc3355->id);
		set_text_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
		for(i=0; i<buflen; i++)
			printf("%02x", *(p++));
		printf("\n");
	}

	ret = write(gc3355->dev_fd, buf, buflen);
	if (ret != buflen)
		return 1;

	return 0;
}

/* read data from UART */
#ifndef WIN32
static int gc3355_gets(struct gc3355_dev *gc3355, unsigned char *buf, int read_count)
{
	int				fd;
	unsigned char	*bufhead, *p;
	fd_set			rdfs;
	struct timeval	tv;
	ssize_t			nread;
	int				read_amount;
	int				n, i;

	fd = gc3355->dev_fd;
	bufhead = buf;
	read_amount = read_count;
	memset(buf, 0, read_count);
	tv.tv_sec  = 1;
	tv.tv_usec = 0;
	while(true) {
		FD_ZERO(&rdfs);
		FD_SET(fd, &rdfs);
		n = select(fd+1, &rdfs, NULL, NULL, &tv);
		if (n < 0) {
			// error occur
			return 1;
		} else if (n == 0) {
			// timeout
			return 2;
		}

		usleep(10000);
		nread = read(fd, buf, read_amount);
		if (nread < 0)
			return 1;
		else if (nread >= read_amount) {
			if (!opt_quiet) {
				p = (unsigned char *)bufhead;
#ifndef WIN32
				printf("[1;31m%d: <<< LTC :[0m ", gc3355->id);
#else
				set_text_color(FOREGROUND_RED);
				printf("%d: <<< LTC : ", gc3355->id);
				set_text_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
				for(i=0; i<read_count; i++)
					printf("%02x", *(p++));
				printf("\n");
			}
			return 0;
		}
		else if (nread > 0) {
			buf += nread;
			read_amount -= nread;
		}
	}
}
#else
static int gc3355_gets(struct gc3355_dev *gc3355, unsigned char *buf, int read_count)
{
	int fd;
	unsigned char	*bufhead, *p;
	ssize_t ret = 0;
	int rc = 0;
	int read_amount;
	int i;

	// Read reply 1 byte at a time
	fd = gc3355->dev_fd;
	bufhead = buf;
	read_amount = read_count;
	while (true) {
		ret = read(fd, buf, 1);
		if (ret < 0)
			return 1;

		if (ret >= read_amount) {
			if (!opt_quiet) {
				p = (unsigned char *)bufhead;
#ifndef WIN32
				printf("[1;31m%d: <<< LTC :[0m ", gc3355->id);
#else
				set_text_color(FOREGROUND_RED);
				printf("%d: <<< LTC : ", gc3355->id);
				set_text_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
				for(i=0; i<read_count; i++)
					printf("%02x", *(p++));
				printf("\n");
			}
			return 0;
		}

		if (ret > 0) {
			buf += ret;
			read_amount -= ret;
			continue;
		}
			
		rc++;
		if (rc >= 10)
			return 2;
	}
}
#endif

static void gc3355_send_cmds(struct gc3355_dev *gc3355, const char *cmds[])
{
	unsigned char	ob[160];
	int				i;

	for(i=0; ; i++) {
		if (cmds[i] == NULL)
			break;
		memset(ob, 0, sizeof(ob));
		hex2bin(ob, cmds[i], sizeof(ob));
		gc3355_write(gc3355, ob, strlen(cmds[i])/2);
		usleep(10000);
	}
	return;
}

static void gc3355_set_core_freq(struct gc3355_dev *gc3355, const char *freq)
{
	int		i, inx=5;
	char	*p;
	char	*cmds[2];

	if (freq != NULL) {
		for(i=0; ;i++) {
			if (str_frequency[i] == NULL)
				break;
			if (strcmp(freq, str_frequency[i]) == 0)
				inx = i;
		}
	}

	cmds[0] = (char *)cmd_frequency[inx];
	cmds[1] = NULL;
	gc3355_send_cmds(gc3355, (const char **)cmds);
	applog(LOG_INFO, "%d: Set GC3355 core frequency to %sMhz", gc3355->id, str_frequency[inx]);
	return;
}

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

/* proxy to cgminer */
static int gc3355_create_proxy(struct gc3355_dev *gc3355)
{
	int sock;
	short port;

	sock = gc3355->sockltc;
	if (sock > 0)
		gc3355_close(sock);
	gc3355->sockltc = -1;

	port = GRIDSEED_LTC_PROXY_PORT;
	while(true) {
		sock = check_udp_port_in_use(port);
		if (sock > 0)
			break;
		port++;
	}
	gc3355->sockltc = sock;
	gc3355->r_port  = port;
	gc3355->s_port  = port - 1000;

	gc3355->toaddr.sin_family = AF_INET;
	gc3355->toaddr.sin_port   = htons(gc3355->s_port);
	gc3355->toaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	applog(LOG_NOTICE, "%d: Create LTC proxy on port %d", gc3355->id, gc3355->r_port);
	return 0;
}

static void gc3355_send_ltc_task(struct gc3355_dev *gc3355, unsigned char *buf, int buflen)
{
	if (gc3355->sockltc <= 0)
		return;
	sendto(gc3355->sockltc, buf, buflen, 0, (struct sockaddr *)&(gc3355->toaddr), sizeof(gc3355->toaddr));
	return;
}

static int gc3355_recv_ltc_nonce(struct gc3355_dev *gc3355, unsigned char *buf, int read_count)
{
	fd_set rdfs;
	struct timeval tv;
	int sock;
	int n;

	sock = gc3355->sockltc;
	if (sock <= 0)
		return 1;

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	FD_ZERO(&rdfs);
	FD_SET(sock, &rdfs);

	n = select(sock+1, &rdfs, NULL, NULL, &tv);
	if (n < 0) {
		// error occur
		return 1;
	} else if (n == 0) {
		// timeout
		return 2;
	}

	n = recvfrom(sock, buf, read_count, 0, NULL, NULL);
	if (n < 0)
		return 1;
	else if (n == 0)
		return 2;
	return 0;
}

/*
 * miner thread
 */
static void *gc3355_thread(void *userdata)
{
	struct thr_info	*mythr = userdata;
	int thr_id = mythr->id;
	struct gc3355_dev gc3355;
	struct work work;
	unsigned char *scratchbuf = NULL;
	char s[16];
	int i;

	gc3355.id = thr_id;
	gc3355.dev_fd = -1;
	gc3355.sockltc = -1;
	gc3355.resend = true;

	scratchbuf = scrypt_buffer_alloc();

	applog(LOG_INFO, "%d: GC3355 chip mining thread started, in %s mode", thr_id, opt_dual ? "DUAL" : "SINGLE");

	if (opt_dual) {
		gc3355_create_proxy(&gc3355);
	} else {
		if (gc3355_open(&gc3355, mythr->devname, B115200))
			return NULL;
		applog(LOG_INFO, "%d: Open UART device %s", thr_id, gc3355_dev);

		gc3355_send_cmds(&gc3355, single_cmd_init);
		gc3355_set_core_freq(&gc3355, opt_frequency);
	}

	while(1) {
		bool	keep_work;
		int		rc;

		if (have_stratum) {
			while (!*g_work.job_id || time(NULL) >= g_work_time + 120)
				sleep(1);
			pthread_mutex_lock(&g_work_lock);
		} else {
			/* obtain new work from internal workio thread */
			pthread_mutex_lock(&g_work_lock);
			if (!(have_longpoll || have_stratum) ||
					time(NULL) >= g_work_time + LP_SCANTIME*3/4) {
				if (unlikely(!get_work(mythr, &g_work))) {
					applog(LOG_ERR, "%d: Work retrieval failed, exiting "
						"mining thread %d", thr_id, mythr->id);
					pthread_mutex_unlock(&g_work_lock);
					goto out_gc3355;
				}
				time(&g_work_time);
			}
			if (have_stratum) {
				pthread_mutex_unlock(&g_work_lock);
				continue;
			}
		}

		if (memcmp(work.data, g_work.data, 76)) {
			memcpy(&work, &g_work, sizeof(struct work));
			keep_work = false;
		} else
			keep_work = true;
		pthread_mutex_unlock(&g_work_lock);
		work_restart[thr_id].restart = 0;

		rc = gc3355_scanhash(&gc3355, work.data, scratchbuf, work.target, keep_work);
		if (rc < 0) {
			gc3355.resend = true;
			applog(LOG_NOTICE, "%d: LTC proxy request new task", thr_id);
			continue;
		}
		if (rc && !submit_work(mythr, &work))
			break;
	}

out_gc3355:
	tq_freeze(mythr->q);
	gc3355_close(gc3355.dev_fd);
	gc3355_close(gc3355.sockltc);
	return NULL;
}

/* scan hash in GC3355 chips */
static int gc3355_scanhash(struct gc3355_dev *gc3355, uint32_t *pdata, unsigned char *scratchbuf,
		const uint32_t *ptarget, bool keep_work)
{
	static char	*indi = "|/-\\";
	static int	i_indi = 0;

	int thr_id = gc3355->id;
	unsigned char bin[160];
	unsigned char rptbuf[12];
	uint32_t data[20], nonce, hash[8];
	uint32_t midstate[8];
	uint32_t n = pdata[19] - 1;
	const uint32_t Htarg = ptarget[7];
	int ret, i;

	memcpy(data, pdata, 80);
	sha256_init(midstate);
	sha256_transform(midstate, data, 0);
#if 0
	printf("[1;33mTarget[0m\n");
	print_hex((unsigned char *)ptarget, 32);
	printf("[1;33mData[0m\n");
	print_hex((unsigned char*)pdata, 80);
	printf("[1;33mMidstate[0m\n");
	print_hex((unsigned char *)midstate, 32);
#endif
	if (!keep_work || gc3355->resend) {
		applog(LOG_INFO, "%d: Dispatching new work to GC3355 LTC core", gc3355->id);
		memset(bin, 0, sizeof(bin));
		memcpy(bin, "\x55\xaa\x1f\x00", 4);
		memcpy(bin+4, (unsigned char *)ptarget, 32);
		memcpy(bin+36, (unsigned char *)midstate, 32);
		memcpy(bin+68, (unsigned char*)pdata, 80);
		memcpy(bin+148, "\xff\xff\xff\xff", 4);
		/* TASK_ID */
		memcpy(bin+152, "\x12\x34\x56\x78", 4);
		if (opt_dual) {
			gc3355_send_ltc_task(gc3355, bin, 156);
		} else {
			gc3355_send_cmds(gc3355, single_cmd_reset);
			usleep(1000);
			gc3355_write(gc3355, bin, 156);
		}
		gc3355->resend = false;
	} else {/* keepwork */
		printf(" %c%c%c%c", indi[i_indi], indi[i_indi], indi[i_indi], 13);
		i_indi++;
		if (i_indi >= strlen(indi))
			i_indi = 0;
		fflush(stdout);
	}

	while(!work_restart[thr_id].restart) {

		if (opt_dual)
			ret = gc3355_recv_ltc_nonce(gc3355, (unsigned char *)rptbuf, 12);
		else
			ret = gc3355_gets(gc3355, (unsigned char *)rptbuf, 12/*bytes*/);
		if (ret ==0 && rptbuf[0] != 0x55)
			continue;
		memcpy((unsigned char *)&nonce, rptbuf+4, 4);

		if (ret == 0) {
			unsigned char s[80], *ph;

			if (memcmp(rptbuf, "\x55\xaa\x55\xaa\x01\x01\x01\x01\x01\x00\x00\x00", 12) == 0)
				return -1; // request to regen new work

			memcpy(pdata+19, &nonce, sizeof(nonce));
			data[19] = nonce;
			scrypt_1024_1_1_256(data, hash, midstate, scratchbuf);

			if (!opt_quiet) {
				ph = (unsigned char *)ptarget;
				for(i=31; i>=0; i--)
					sprintf(s+(31-i)*2, "%02x", *(ph+i));
#ifndef WIN32
				applog(LOG_DEBUG, "%d: Target: [1;34m%s[0m", gc3355->id, s);
#else
				set_text_color(FOREGROUND_BLUE);
				applog(LOG_DEBUG, "%d: Target: %s", gc3355->id, s);
				set_text_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif

				ph = (unsigned char *)hash;
				for(i=31; i>=0; i--)
					sprintf(s+(31-i)*2, "%02x", *(ph+i));
#ifndef WIN32
				applog(LOG_DEBUG, "%d:   Hash: [1;34m%s[0m", gc3355->id, s);
#else
				set_text_color(FOREGROUND_BLUE);
				applog(LOG_DEBUG, "%d:   Hash: %s", gc3355->id, s);
				set_text_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
			}

			ph = (unsigned char *)&nonce;
			for(i=0; i<4; i++)
				sprintf(bin+i*2, "%02x", *(ph++));

			if (hash[7] <= Htarg && fulltest(hash, ptarget)) {
#ifndef WIN32
				applog(LOG_INFO, "%d: Got nonce %s, [1;32mHash <= Htarget![0m", gc3355->id, bin);
#else
				set_text_color(FOREGROUND_GREEN);
				applog(LOG_INFO, "%d: Got nonce %s, Hash <= Htarget!", gc3355->id, bin);
				set_text_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
				return 1;
			} else {
#ifndef WIN32
				applog(LOG_INFO, "%d: Got nonce %s, [1;31mInvalid nonce![0m", gc3355->id, bin);
#else
				set_text_color(FOREGROUND_RED);
				applog(LOG_INFO, "%d: Got nonce %s, Invalid nonce!", gc3355->id, bin);
				set_text_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
				break;
			}
		} else
			break;
	}
	return 0;
}

/*
 * create miner thread
 */
static int create_gc3355_miner_threads(struct thr_info *thr_info, int opt_n_threads)
{
	struct thr_info *thr;
	unsigned char *pd, *p;
	int i;

	if (opt_dual) {
		thr = &thr_info[0];
		thr->id = 0;
		thr->q = tq_new();
		if (!thr->q)
			return 1;
		if (unlikely(pthread_create(&thr->pth, NULL, gc3355_thread, thr))) {
			applog(LOG_ERR, "%d: GC3355 chip mining thread create failed", thr->id);
			return 1;
		}
		return 0;
	}

	pd = gc3355_dev;
	/* start GC3355 chip mining thread */
	for (i=0; i<opt_n_threads; i++) {
		thr = &thr_info[i];
		thr->id = i;
		thr->q = tq_new();
		if (!thr->q)
			return 1;

		p = strchr(pd, ',');
		if (p != NULL)
			*p = '\0';
		thr->devname = strdup(pd);
		pd = p + 1;

		if (unlikely(pthread_create(&thr->pth, NULL, gc3355_thread, thr))) {
			applog(LOG_ERR, "%d: GC3355 chip mining thread create failed", thr->id);
			return 1;
		}
		usleep(100000);
	}
	return 0;
}
