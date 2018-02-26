#ifndef __SLLIN_APP_H__
#define __SLLIN_APP_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <poll.h>
#include <ctype.h>
#include <libgen.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <pthread.h>

#define LIN_TIMEOUT 5000 //5s
#define OilTempOffset 40

enum {
	STATUS_OK = 0,
	STATUS_KO = -1,
	STATUS_UNDEF = -5
};

enum {
    LinFuelLvl = 0,
	LinOilPres,
	LinOilTemp,
	LinSpeedo,
	LinBackLight,
	LinSlavesNum
};

struct LinMsg{
	unsigned long ulLinId;
	unsigned char ucDataLen;
	unsigned char pucData[8];
};

struct LinMsgStruct{
	unsigned long  uwLinId;
	unsigned char  ucLinDataLen;
	signed   long  slMinVal;
    unsigned long  ulMaxVal;
};

#endif
