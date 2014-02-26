#ifndef INCLUDE_DRIVER_GRIDSEED_H
#define INCLUDE_DRIVER_GRIDSEED_H

#ifdef USE_GRIDSEED

#include "util.h"

#define GRIDSEED_MINER_THREADS		1
#define GRIDSEED_LATENCY			4

#define GRIDSEED_DEFAULT_BAUD		115200
#define GRIDSEED_DEFAULT_FREQUENCY	"600"
#define GRIDSEED_DEFAULT_CHIPS		5
#define GRIDSEED_DEFAULT_MODULES	1
#define GRIDSEED_DEFAULT_USEFIFO	0
#define GRIDSEED_DEFAULT_BTCORE		16

#define GRIDSEED_COMMAND_DELAY		20
#define GRIDSEED_READ_SIZE			12
#define GRIDSEED_MCU_QUEUE_LEN		8
#define GRIDSEED_SOFT_QUEUE_LEN		(GRIDSEED_MCU_QUEUE_LEN+2)
#define GRIDSEED_READBUF_SIZE		8192

#define GRIDSEED_PROXY_PORT			3350

#define transfer(gridseed, request_type, bRequest, wValue, wIndex, cmd) \
		_transfer(gridseed, request_type, bRequest, wValue, wIndex, NULL, 0, cmd)

#endif

#endif /* INCLUDE_DRIVER_GRIDSEED_H */
