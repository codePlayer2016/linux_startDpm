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

#include <setjmp.h>

#include "jpeglib.h"
#define IMG_SIZE4_DSP (0x100000)
#define PAGE_SIZE (0x1000)
#define WAITTIME (0x07FFFFFF)
#define WTBUFLENGTH (2*4*1024)
#define RDBUFLENGTH (4*1024*1024-4*4*1024)
#define URL_ITEM_SIZE (102)
#define BMP_ALIGN (4)
#define END_FLAG  (0xffaa)

#define JPEG_QUALITY 100 //图片质量

typedef struct _tagArguments
{
	char *outPutPath;
	char *pDevicePath;
	char *pModel;
} Arguments;

typedef struct
{
	char type1;
	char type2;
} BmpFileHead;

typedef struct
{
	unsigned int imageSize;
	unsigned int blank;
	unsigned int startPosition;
	unsigned int length;
	unsigned int width;
	unsigned int height;
	unsigned short colorPlane;
	unsigned short bitColor;
	unsigned int zipFormat;
	unsigned int realSize;
	unsigned int xPels;
	unsigned int yPels;
	unsigned int colorUse;
	unsigned int colorImportant;
} BmpInfoHead;

typedef struct __tagSubPicInfor
{
	uint8_t *subPicAddr[100];
	uint32_t subPicLength[100];
	uint8_t subPicNums;
	uint32_t subWidth[100];
	uint32_t subHeight[100];
	uint32_t subXpoint[100];
	uint32_t subYpoint[100];
} SubPicInfor;
SubPicInfor sPictureInfor;
typedef struct __tagOriginalPicInfor
{
	uint8_t jpegName[100][40];
	uint8_t urlString[100][120];
	uint32_t originalPicLength[100];
	uint8_t *originalPicAddr[100];
} OriginalPicInfor;
OriginalPicInfor oPictureInfor;

typedef struct fileName
{
	uint8_t name[100][100];
} BmpFileName;
BmpFileName bmpFileName;
void showHelp(int retVal);
void showError(int retVal);
int parseArguments(int argc, char **argv, Arguments* pArguments);
int startDpm(Arguments* pArguments);
int bgr2rgb(char* pImgData, int imgWidth, int imgHeight);

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

int rgb2jpeg(char *pRgbData, int rgbWidth, int rgbHeigth, FILE *outfile)
{
	int retVal = 0;
	int indexWidth = 0;

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_height = rgbHeigth;
	cinfo.image_width = rgbWidth;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	//jpeg_set_quality(&cinfo, JPEG_QUALITY, TRUE );
	jpeg_start_compress(&cinfo, TRUE);

	JSAMPROW row_pointer[1]; /* pointer to a single row */
	int row_stride; /* physical row width in buffer */

	row_stride = cinfo.image_width * 3; /* JSAMPLEs per row in image_buffer */

	bgr2rgb(pRgbData, rgbWidth, rgbHeigth);

	for (indexWidth = 0; indexWidth < cinfo.image_height; indexWidth++)
	{
		row_pointer[0] =
				(JSAMPROW) (pRgbData + cinfo.next_scanline * row_stride);
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	fclose(outfile);
	return (retVal);

}
int saveOriginalPicture(char *pRgbData, int originalLength, FILE *oriOutFile)
{
	int retVal = 0;
	int writeLength = 0;

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	writeLength = fwrite(pRgbData, originalLength, 1, oriOutFile);
	if (writeLength <= 0)
	{
		fprintf(stderr, "file fwrite error\n");
		retVal = -1;
		exit(1);
	}
	fclose(oriOutFile);
	return (retVal);
}
int saveUrlToManifest(char *urlName, int xPoint, int yPoint, int width,
		int height, FILE *manOutFile)
{
	int retVal = 0;
	char x[4], y[4], w[4], h[4];
	char locPosition[100];

	sprintf(x, "%d", xPoint);
	sprintf(y, "%d", yPoint);
	sprintf(w, "%d", width);
	sprintf(h, "%d", height);
	strcpy(locPosition, x);
	strcat(locPosition, ",");
	strcat(locPosition, y);
	strcat(locPosition, ",");
	strcat(locPosition, w);
	strcat(locPosition, ",");
	strcat(locPosition, h);
	//write urlName
	fputs(urlName, manOutFile);
	fputc('\n', manOutFile);
	//write location position
	fputs(locPosition, manOutFile);

	fclose(manOutFile);
	return (retVal);

}
int bgr2rgb(char* pImgData, int imgWidth, int imgHeight)
{
	int widthIndex = 0;
	int heigthIndex = 0;
	unsigned char tempVal = 0;
	char *pRow = NULL;

	for (heigthIndex = 0; heigthIndex < imgHeight; heigthIndex++)
	{
		pRow = (pImgData + (heigthIndex * imgWidth * 3));
		for (widthIndex = 0; widthIndex < imgWidth; widthIndex++)
		{
			tempVal = pRow[0];
			pRow[0] = pRow[2];
			pRow[2] = tempVal;
			pRow += 3;
		}
	}
	return (0);
}
int rgb2bmp(char* imgData, int imgWidth, int imgHeight, int picNum)
{
	int imgWidthStep = 3 * imgWidth;
	int channels = 3;

	char *pdst = (char *) malloc(imgWidth * imgHeight * 3 + 54);
	if (pdst == NULL)
		printf("ERROR RgbBuf\n");
	char *addr = pdst;
	BmpFileHead bmpfilehead =
	{ 'B', 'M' };
	BmpInfoHead bmpinfohead;
	int step = imgWidth * channels; //windowsÎ»ÍŒstep±ØÐëÊÇ4µÄ±¶Êý
	int modbyte = step & (BMP_ALIGN - 1); //
	if (modbyte != 0)
		step += (BMP_ALIGN - modbyte);

	memcpy(addr, &bmpfilehead, sizeof(BmpFileHead));
	addr += sizeof(BmpFileHead); //move to next section

	bmpinfohead.blank = 0;
	bmpinfohead.startPosition = sizeof(BmpFileHead) + sizeof(BmpInfoHead);
	bmpinfohead.realSize = step * imgHeight;
	bmpinfohead.imageSize = bmpinfohead.realSize + bmpinfohead.startPosition;
	bmpinfohead.length = 0x28;
	bmpinfohead.width = imgWidth;
	bmpinfohead.height = imgHeight;
	bmpinfohead.colorPlane = 0x01;
	bmpinfohead.bitColor = 8 * channels;
	bmpinfohead.zipFormat = 0;
	bmpinfohead.xPels = 0xEC4;
	bmpinfohead.yPels = 0xEC4;
	bmpinfohead.colorUse = 0;
	bmpinfohead.colorImportant = 0;

	memcpy(addr, &bmpinfohead, sizeof(BmpInfoHead));
	addr += sizeof(BmpInfoHead); //move to next section
	char *pdata = imgData;
	int idx;
	for (idx = imgHeight - 1; idx >= 0; idx--)
	{
		memcpy(addr, pdata, step);
		addr += step; //move to next section
		pdata += imgWidthStep;
	}

	sprintf(bmpFileName.name[picNum], "test_%d.bmp", picNum);
	FILE* p = fopen(((char*) bmpFileName.name[picNum]), "wb+");
	fwrite(pdst, 1, imgWidth * imgHeight * 3 + 54, p);
	fclose(p);
	free(pdst);
	return bmpinfohead.imageSize;
}
int GetDpmProcessPic(uint32_t *srcBuf, int fdDevice, Arguments* pArguments)
{
	int retVal = 0;
	int retIoVal = 0;
	sPictureInfor.subPicNums = 0;
	int picNum = 0;
	FILE *oriOutFile = NULL;
	FILE *subOutFile = NULL;
	FILE *manOutFile = NULL;

	char oJpegName[40];
	char sJpegName[40];
	uint32_t *pSrc = srcBuf;
	while ((*pSrc) != END_FLAG)
	{
		/***********************get original picture******************************/

		memcpy(oPictureInfor.urlString[picNum], pSrc, 120);
		pSrc = (pSrc + 120 / 4);

		memcpy(oPictureInfor.jpegName[picNum], pSrc, 40);
		pSrc = (pSrc + 40 / 4);

		memcpy(&oPictureInfor.originalPicLength[picNum], pSrc, 4);
		pSrc += 1;

		oPictureInfor.originalPicAddr[picNum] = (uint8_t *) malloc(
				oPictureInfor.originalPicLength[picNum] * sizeof(char));
		memcpy(oPictureInfor.originalPicAddr[picNum], ((uint8_t *) pSrc),
				oPictureInfor.originalPicLength[picNum]);
		pSrc = (pSrc + (oPictureInfor.originalPicLength[picNum] + 4) / 4);

		/***********************get sub picture******************************/
		memcpy(&sPictureInfor.subWidth[picNum], pSrc, sizeof(int));
		pSrc += 1;

		memcpy(&sPictureInfor.subHeight[picNum], pSrc, sizeof(int));
		pSrc += 1;

		memcpy(&sPictureInfor.subXpoint[picNum], pSrc, sizeof(int));
		pSrc += 1;

		memcpy(&sPictureInfor.subYpoint[picNum], pSrc, sizeof(int));
		pSrc += 1;

		memcpy(&sPictureInfor.subPicLength[picNum], pSrc, sizeof(int));
		pSrc += 1;

		sPictureInfor.subPicAddr[picNum] = (uint8_t *) malloc(
				sPictureInfor.subPicLength[picNum] * sizeof(char));
		memcpy(sPictureInfor.subPicAddr[picNum], ((uint8_t *) pSrc),
				sPictureInfor.subPicLength[picNum]);

		pSrc = (pSrc + (sPictureInfor.subPicLength[picNum] + 4) / 4);

		//formatting and save output picture,create the outputDir.

		if (opendir(oPictureInfor.jpegName[picNum]) != NULL)
		{
			retVal = chdir(oPictureInfor.jpegName[picNum]);
			if (retVal == 0)
			{
				remove("0.jpg");
				remove("1.jpg");
				remove("manifest.txt");
			}
			retVal = chdir("..");
			retVal = remove(oPictureInfor.jpegName[picNum]);
			if (retVal == 0)
			{
			}
			else
			{
				printf("remove error\n");
			}
		}
		else
		{
			//directory is not exist,then we need to mkdir
		}
		retVal = mkdir(oPictureInfor.jpegName[picNum], "0777");
		if (retVal == 0)
		{
			retVal = chmod(oPictureInfor.jpegName[picNum],
					S_IROTH | S_IWOTH | S_IXOTH);
			if (retVal == 0)
			{
			}
			else
			{
				retVal = -4;
				printf("chmod the outDir error\n");
			}
			retVal = chdir(oPictureInfor.jpegName[picNum]);
			if (retVal == 0)
			{
				if ((oriOutFile = fopen("0.jpg", "wb")) == NULL)
				{
					fprintf(stderr, "can't open 0.jpg\n");
					retVal = -1;
					exit(1);
				}
				if ((subOutFile = fopen("1.jpg", "wb")) == NULL)
				{
					fprintf(stderr, "can't open 1.jpg\n");
					retVal = -1;
					exit(1);
				}
				if ((manOutFile = fopen("manifest.txt", "wb")) == NULL)
				{
					fprintf(stderr, "can't open manifest\n");
					retVal = -1;
					exit(1);
				}
				//return to out dir
				retVal = chdir("..");
			}
			else
			{
				printf("chdir picoutDir failed\n");
				retVal = -3;
			}
		}
		else
		{
			printf("mkdir picoutDir error\n");
		}

		//save original picture
		saveOriginalPicture((char *) oPictureInfor.originalPicAddr[picNum],
				oPictureInfor.originalPicLength[picNum], oriOutFile);
		//save sub picture
		rgb2jpeg((char *) sPictureInfor.subPicAddr[picNum],
				sPictureInfor.subWidth[picNum], sPictureInfor.subHeight[picNum],
				subOutFile);
		//save location position
		saveUrlToManifest(oPictureInfor.urlString[picNum],
				sPictureInfor.subXpoint[picNum],
				sPictureInfor.subYpoint[picNum], sPictureInfor.subWidth[picNum],
				sPictureInfor.subHeight[picNum], manOutFile);
		sPictureInfor.subPicNums++;
		picNum++;
		free(oPictureInfor.originalPicAddr[picNum]);
		free(sPictureInfor.subPicAddr[picNum]);

	}
	//5.20 clear zone Inbuffer
	memset(srcBuf, 0, RDBUFLENGTH);

	return retVal;
}
int startDpm(Arguments* pArguments)
{
	int retVal = 0;
	int retIoVal = 0;
	float timeElapse = 0;
	int ovConfig = 0;

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
	int one = 1;
	int two = 2;
	int three = 3;
	uint32_t *pModelType = NULL;

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
#if 1
			pModelType = (uint32_t *) ((uint8_t *) g_pMmapAddr + PAGE_SIZE
					+ 4 * sizeof(int));
			if (strcmp(pArguments->pModel, "motor") == 0)
			{
				*pModelType = one;
			}
			if (strcmp(pArguments->pModel, "car") == 0)
			{
				*pModelType = two;
			}
			if (strcmp(pArguments->pModel, "person") == 0)
			{
				*pModelType = three;
			}
#endif

		}
		else
		{
			printf("mmap failed\n");
			retVal = -5;
		}
	}

	waitWriteBufferReadyParam.waitType = LINKLAYER_IO_START;
	waitWriteBufferReadyParam.pendTime = WAITTIME;
	waitWriteBufferReadyParam.pBufStatus = &status;
	retIoVal = ioctl(fdDevice, DPU_IO_CMD_WAITDPMSTART,
			&waitWriteBufferReadyParam); // dsp should init the RD register to empty in DSP.
	//clear dpmOver reg
	if (retVal == 0)
	{
		printf("clear dpmOver reg");
		ovConfig = LINKLAYER_IO_OVER;
		retIoVal = ioctl(fdDevice, DPU_IO_CMD_CHANGOVERREG, &ovConfig);

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
	while (1)
	{
		//set dpmStart reg
		if (retVal == 0)
		{
			printf("change dpmstart reg \n");
			int stConfig = LINKLAYER_IO_START;
			retIoVal = ioctl(fdDevice, DPU_IO_CMD_CHANGEREG, &stConfig);
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

		// interrupt to DSP.
		if (retVal == 0)
		{
			printf("trigger interrupt to DSP\n");
			pInterruptPollParams->interruptAndPollDirect = 0;

			retIoVal = ioctl(fdDevice, DPU_IO_CMD_INTERRUPT,
					pInterruptPollParams);

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

			retIoVal = ioctl(fdDevice, DPU_IO_CMD_WAITDPM,
					pInterruptPollParams);
			if (retIoVal != -1)
			{

				printf("the Dpm finished\n");
				gettimeofday(&downloadEnd, NULL);
				timeElapse = ((downloadEnd.tv_sec - downloadStart.tv_sec)
						* 1000000
						+ (downloadEnd.tv_usec - downloadStart.tv_usec));
				printf("the dpm elapse %f ms\n", timeElapse / 1000);
				//get dpm sub picture info
				GetDpmProcessPic(pLinkLayerBuffer->pInBuffer, fdDevice,
						pArguments);
				//after pc get data,we should set pc_over_ctl to notify dsp,pc have get finish data
				ovConfig = LINKLAYER_IO_OVER;
				retIoVal = ioctl(fdDevice, DPU_IO_CMD_CHANGOVERREG, &ovConfig);

				if (retIoVal != -1)
				{
					//printf("clear dpmOver reg success\n");
				}
				else
				{
					retVal = -7;
					printf("clear dpmOver reg failed\n");
				}

				//send interrupt to dsp
				printf(
						"trigger interrupt to DSP and notify pc have finished data\n");
				pInterruptPollParams->interruptAndPollDirect = 0;

				retIoVal = ioctl(fdDevice, DPU_IO_CMD_INTERRUPT,
						pInterruptPollParams);

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
			else
			{
				retVal = -8;
				printf("ioctl for DPU_IO_CMD_WAITDPM failed\n");
			}

		}
		waitWriteBufferReadyParam.pBufStatus = &status;
		retIoVal = ioctl(fdDevice, DPU_IO_CMD_CHECKDPMALLOVER,
				&waitWriteBufferReadyParam); // dsp should init the RD register to empty in DSP.
		if (status == 0)
		{
			printf("pc have check dpm all over,so break while\n");
			break;
		}
	} //while

	// release the resource.
	munmap(g_pMmapAddr, mmapAddrLength);
	free(pLinkLayerBuffer);
	close(fdDevice);
	return (retVal);
}

