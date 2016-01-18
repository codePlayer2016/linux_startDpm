/*
 * LinkLayer.h
 *
 *  Created on: Sep 25, 2015
 *      Author: spark
 */

#ifndef INC_LINKLAYER_H_
#define INC_LINKLAYER_H_
#if 0
typedef struct _tagRegisterTable
{
	uint32_t inBufferStatus;
	uint32_t outBufferStatus;
	uint32_t reserved[0x1000/4-2];
} RegisterTable;

typedef struct _tagLinkLayerHandler
{
	int fdDevice;
	RegisterTable *pRegisterTable;
	uint32_t *pInBuffer;
	uint32_t *pAddrReadFromDsp;
	uint32_t addrLength4WriteToDsp;
	uint32_t addrLength4ReadFromDsp;
}LinkLayerHandler,*LinkLayerHandlerPtr;
# endif
# if 0
typedef struct _tagLinkLayerHandler
{
	struct RegistersTable
	{
		uint32_t IRQStatus;
		uint32_t InBufferStatus;
		uint32_t OutBufferStatus;
		uint32_t reserved[0x1000/4-3];
	} *pRegistersTable;

}LinkLayerHandler,*LinkLayerHandlerPtr;
#endif
/*
// init
pThis=(*)malloc(sizeof(LinkerLayer));
pThis->pRegistersTable=mmap(0-0x1000);

// method
LL_method(pThis, args...){
	LinkLayerHandler::RegistersTable *regTable=
	const uint32_t* irqStatus = pThis->pRegistersTable.IRQStatus
pThis->pRegistersTable.IRQStatus&ddd=0;
}
*/
#if 0
int linkLayerPCRead(uint8_t *pDestBuffer, uint32_t dataLength);
int LinkLayer_Write(uint8_t *pSrcBuffer, uint32_t dataLength);

int linkLayerPCRead(uint8_t *pDestBuffer, uint32_t dataLength,
		LinkLayerHandler *pLlHandle);
int LinkLayer_Write(uint8_t *pSrcBuffer, uint32_t dataLength,
		LinkLayerHandler *pLlHandle);
#endif

typedef struct _tagLinkLayerBuffer
{
	uint32_t *pInBuffer;
	uint32_t *pOutBuffer;
	uint32_t inBufferLength;
	uint32_t outBufferLength;
}LinkLayerBuffer, *LinkLayerPtr;
#endif /* INC_LINKLAYER_H_ */
