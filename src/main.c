/*
 * startDpm.c
 *
 *  Created on: Dec 10, 2015
 *      Author: root
 */

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<unistd.h>
#include <string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<string.h>
#include"DPU_ioctl.h"
#include "LinkLayer.h"

#define IMG_SIZE4_DSP (0x100000)
#define PAGE_SIZE (0x1000)
#define WAITTIME (0x07FFFFFF)
#define WTBUFLENGTH (2*4*1024)
#define RDBUFLENGTH (4*1024*1024-4*4*1024)
#define URL_ITEM_SIZE (102)

typedef struct _tagArguments
{
	char *outPutPath;
	char *pDevicePath;
	char *pModel;
} Arguments;

void showHelp(int retVal);
void showError(int retVal);
int parseArguments(int argc, char **argv, Arguments* pArguments);
int startDpm(Arguments* pArguments);

int main(int argc, char **argv)
{
	// TODO: -f <url list file>
	// TODO: -t <device>
	// TODO: --help
	int retVal = 0;
	Arguments arguments;

	retVal = parseArguments(argc, argv, &arguments);
	if (retVal == 0)
	{
		retVal = startDpm((Arguments *) &arguments);
		showError(retVal);
	}
	else
	{
		showHelp(retVal);
	}

	return (retVal);
}

void showHelp(int retVal)
{
	printf("--help\n");
	printf("input the command as follow: ");
	printf("start-dpm -o <outputPath> -t <device> -m <algorithm model>\n");
	printf("-o <outputPath>\n");
	printf("\t specify the output path\n");
	printf("-t <device>\n");
	printf("\t specify the target device\n");
	printf("-m <algorithm model>\n");
	printf("\t specify the algorithm model\n");
}

void showError(int retVal)
{

}

int parseArguments(int argc, char **argv, Arguments* pArguments)
{
	int retVal = 0;

	if (argc == 7)
	{
		if (strcasecmp("-o", argv[1]) == 0)
		{
			pArguments->outPutPath = argv[2];
			printf("the output file is %s\n", pArguments->outPutPath);
		}
		else
		{
			retVal = -1;
			printf("error:out file is %s\n", argv[2]);
		}

		if ((retVal == 0) && (pArguments->outPutPath != NULL))
		{
			if (strcasecmp("-t", argv[3]) == 0)
			{
				pArguments->pDevicePath = argv[4];
				printf("the target device is %s\n", pArguments->pDevicePath);
			}
			else
			{
				retVal = -2;
				printf("error:the device is %s\n", argv[4]);
			}
		}

		if ((retVal == 0) && (pArguments->pDevicePath != NULL))
		{
			if (strcasecmp("-m", argv[5]) == 0)
			{
				pArguments->pModel = argv[6];
			}
			else
			{
				retVal = -3;
				printf("error:the algorithm mode is %s", pArguments->pModel);
			}
		}

	}
	else
	{
		printf("error:the para number is %d\n", argc);
		retVal = -3;
	}
	if (retVal != 0)
	{
		showHelp(retVal);
	}
	return (retVal);
}

int startDpm(Arguments* pArguments)
{
	int retVal = 0;
	int retIoVal = 0;
	float timeElapse = 0;

	struct timeval downloadStart;
	struct timeval downloadEnd;
	int fdDevice = 0;
	FILE *fpUrlList = NULL;
	const char dev_name[] = "/dev/DPU_driver_linux";
	char arrayUrlList[50 * 102];
	char *pArrayUList = arrayUrlList;
	int urlItmeNum = 0;
	int wtConfig = 0;
	uint32_t *g_pMmapAddr = NULL;
	uint32_t mmapAddrLength = (4 * 1024 * 1024);
	DPUDriver_WaitBufferReadyParam waitWriteBufferReadyParam;
	int status = -1;
	int result = -1;
	uint32_t *pUrlNums = NULL;
	uint32_t *pDownloadPicNums = NULL;
	uint32_t *pFailedPicNUms = NULL;

	LinkLayerBuffer *pLinkLayerBuffer = (LinkLayerBuffer *) malloc(
			sizeof(LinkLayerBuffer));
	interruptAndPollParam interruptPollParams;
	interruptAndPollParam *pInterruptPollParams =
			(interruptAndPollParam *) &interruptPollParams;

	if (pLinkLayerBuffer != NULL)
	{
		pLinkLayerBuffer->inBufferLength = RDBUFLENGTH; //
		pLinkLayerBuffer->outBufferLength = WTBUFLENGTH; //8k
	}
	else
	{
		retVal = -1;
	}
	// fopen the device.
	if (retVal == 0)
	{
		fdDevice = open(dev_name, O_RDWR);
		if (fdDevice >= 0)
		{
			printf("the open device is %s\n", dev_name);
		}
		else
		{
			printf("pcie device open error\n");
			retVal = -2;
		}
	}
	// create the outputDir.
	if (retVal == 0)
	{
		retVal = access(pArguments->outPutPath, 0);
		if (retVal == 0)
		{

		}
		else
		{
			retVal = mkdir(pArguments->outPutPath, "0777");
			if (retVal == 0)
			{
				printf("create outDir successful\n");
			}
			else
			{
				printf("create outDir failed\n");
				retVal = -3;
			}
		}
	}

	// change the Dir authority.
	if (retVal == 0)
	{
		retVal = chmod(pArguments->outPutPath, S_IROTH | S_IWOTH | S_IXOTH);
		if (retVal == 0)
		{
		}
		else
		{
			retVal = -4;
			printf("chmod the outDir error\n");
		}
	}

	// mmap and get the registers.
	if (retVal == 0)
	{
		g_pMmapAddr = (uint32_t *) mmap(NULL, mmapAddrLength,
				PROT_READ | PROT_WRITE, MAP_SHARED, fdDevice, 0);
		// polling the dsp can be written to.
		if ((int) g_pMmapAddr != -1)
		{
			printf("mmap finished\n");
			pLinkLayerBuffer->pOutBuffer = (uint32_t *) ((uint8_t *) g_pMmapAddr
					+ PAGE_SIZE * 2);
			pLinkLayerBuffer->pInBuffer = (uint32_t *) ((uint8_t *) g_pMmapAddr
					+ PAGE_SIZE * 2 * 2);

			pUrlNums = (uint32_t *) ((uint8_t *) g_pMmapAddr + PAGE_SIZE
					+ 3 * sizeof(4));
			pDownloadPicNums = (uint32_t *) ((uint8_t *) g_pMmapAddr
					+ 3 * sizeof(4));

			pFailedPicNUms = (uint32_t *) ((uint8_t *) g_pMmapAddr
					+ 4 * sizeof(4));
			printf(
					"g_pMmapAddr=0x%x,pDownloadPicNums=0x%x,pDownloadPicNums=0x%x\n",
					g_pMmapAddr, pDownloadPicNums, pFailedPicNUms);

			waitWriteBufferReadyParam.waitType = LINKLAYER_IO_WRITE;
			waitWriteBufferReadyParam.pendTime = WAITTIME;
			waitWriteBufferReadyParam.pBufStatus = &status;
			retIoVal = ioctl(fdDevice, DPU_IO_CMD_WAITBUFFERREADY,
					&waitWriteBufferReadyParam); // dsp should init the RD register to empty in DSP.
		}
		else
		{
			printf("mmap failed\n");
			retVal = -5;
		}
	}
	//set dpmStart reg
	if (retVal == 0)
	{
		printf("change dpmstart reg \n");
		retIoVal = ioctl(fdDevice, DPU_IO_CMD_CHANGEREG, NULL);
		if (retIoVal != -1)
		{
			printf("change dpmstart reg over\n");
		}
		else
		{
			retVal = -6;
			printf("ioctl for change dpmstart reg error\n");
		}
	}
	//clear dpmOver reg
	if (retVal == 0)
	{
		printf("clear dpmOver reg");

		retIoVal = ioctl(fdDevice, DPU_IO_CMD_CLRINTERRUPT, NULL);

		if (retIoVal != -1)
		{
			printf("clear dpmOver reg success\n");
		}
		else
		{
			retVal = -7;
			printf("clear dpmOver reg failed\n");
		}
	}
	// interrupt to DSP.
	if (retVal == 0)
	{
		printf("trigger interrupt to DSP\n");
		pInterruptPollParams->interruptAndPollDirect = 0;

		retIoVal = ioctl(fdDevice, DPU_IO_CMD_INTERRUPT, pInterruptPollParams);

		if (retIoVal != -1)
		{
			printf("trigger the DSP over\n");
		}
		else
		{
			retVal = -7;
			printf("ioctl for interrupt error\n");
		}
	}

	// wait the DSP dpm process over and get the information.
	if (retVal == 0)
	{

		retIoVal = ioctl(fdDevice, DPU_IO_CMD_WAITDPM, pInterruptPollParams);
		if (retIoVal != -1)
		{

			printf("the Dpm finished\n");
			gettimeofday(&downloadEnd, NULL);
			timeElapse = ((downloadEnd.tv_sec - downloadStart.tv_sec) * 1000000
					+ (downloadEnd.tv_usec - downloadStart.tv_usec));
			printf("the dpm elapse %f ms\n", timeElapse / 1000);

		}
		else
		{
			retVal = -8;
			printf("ioctl for DPU_IO_CMD_DPM_TIMEOUT failed\n");
		}

	}

// release the resource.
	munmap(g_pMmapAddr, mmapAddrLength);
	free(pLinkLayerBuffer);
	close(fdDevice);
	return (retVal);
}

