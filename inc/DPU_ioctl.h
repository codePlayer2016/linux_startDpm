#ifndef _TEST_CMD_H
#define _TEST_CMD_H

//#include <sys/ioctl.h>
#include <linux/ioctl.h>
#if 0
typedef enum IO_CMD
{
	IO_CMD_START = 0X100,
	DPU_IO_CMD_WAITBUFFERREADY,
	DPU_IO_CMD_CONFIRM,
	IO_CMD_END
}IO_CMD;
#endif

typedef enum __tagLINKLAYER_IO_TYPE
{
	LINKLAYER_IO_READ = 0,
	LINKLAYER_IO_WRITE = 1,
	LINKLAYER_IO_READ_FIN = 2,
	LINKLAYER_IO_WRITE_FIN = 3,
	LINKLAYER_IO_START = 4,
	LINKLAYER_IO_OVER = 5
} LINKLAYER_IO_TYPE;

typedef struct _tagDPUDriver_WaitBufferReadyParam
{
	int waitType;
	uint32_t pendTime;
	int32_t *pBufStatus;
} DPUDriver_WaitBufferReadyParam, *DPUDriver_WaitBufferReadyParamPtr;

typedef struct _tagInterruptAndPollParam
{
	int interruptAndPollDirect;
	int interruptAndPollResult;
	uint32_t pollTime;
} interruptAndPollParam, *pInterruptAndPollParam;

#define  NULL ((void *)0)
#define READBUFFER_FULL (0x55aa55aa)
#define WRITEBUFFER_EMPTY (0xaa55aa55)
#define OUTBUF_MAX_LENGTH (0x1000)

#define FIND_PCIEDEV_MAXCOUNT (255)

#define IMG_SIZE4_DSP (0x100000)

#if 1
#define PCIEDRIVER_MAGIC  'x'

#define DPU_IO_CMD_WAITBUFFERREADY _IOWR(PCIEDRIVER_MAGIC,0x103,DPUDriver_WaitBufferReadyParam)
#define DPU_IO_CMD_CONFIRM _IOW(PCIEDRIVER_MAGIC,0x104,LINKLAYER_IO_TYPE)
#define DPU_IO_CMD_CHANGEBUFFERSTATUS _IOW(PCIEDRIVER_MAGIC,0x105,LINKLAYER_IO_TYPE)

#define DPU_IO_CMD_INTERRUPT _IOWR(PCIEDRIVER_MAGIC,0x106,interruptAndPollParam)
#define DPU_IO_CMD_WAITDPM _IOWR(PCIEDRIVER_MAGIC,0x107,interruptAndPollParam)

#define  DPU_IO_CMD_WAITDPMSTART _IOWR(PCIEDRIVER_MAGIC,0x110,DPUDriver_WaitBufferReadyParam)
#define DPU_IO_CMD_CHANGEREG _IOW(PCIEDRIVER_MAGIC,0x111,LINKLAYER_IO_TYPE)
#define DPU_IO_CMD_CHANGOVERREG _IOWR(PCIEDRIVER_MAGIC,0x112,LINKLAYER_IO_TYPE)
#endif
#endif//_TEST_CMD_H
