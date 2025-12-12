#include <stdlib.h>

#include "MiniRTOS.h"

/**
 * @brief 申请内存块的最小值
 */
#define heapMINIMUM_BLOCK_SIZE ((size_t)(xHeapStructSize << 1))

/**
 * @brief 为堆分配的内存
 */
#define configTOTAL_HEAP_SIZE ((size_t)(32 * 1024))
static uint8_t ucHeap[configTOTAL_HEAP_SIZE];

/**
 * @brief 组织空闲内存块的链表
 */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;
	size_t xBlockSize;
} BlockLink_t;

/**
 * @brief BlockLink_t结构体的大小。八字节对齐
 */
static const size_t xHeapStructSize	= ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/**
 * @note 用于标记链表的起始和结束位置
 */
static BlockLink_t xStart, *pxEnd = NULL;

/**
 * @note 用于跟踪堆的剩余空间，但并没提及碎片问题
 */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;

/**
 * @note size_t类型的最高位，当BlockLink_t中的xBlockSize成员的此位被设置时，该块属于应用程序。而当
 *       该位为空闲状态时，该块仍属于可用堆空间的一部分
 */
static size_t xBlockAllocatedBit = 0;

/**
 * @brief 首次调用pvPortMalloc()时，会自动调用此函数来设置所需的堆结构
 */
static void prvHeapInit(void)
{
	BlockLink_t *pxFirstFreeBlock;
	uint8_t *pucAlignedHeap;
	size_t uxAddress;
	size_t xTotalHeapSize = configTOTAL_HEAP_SIZE;

	uxAddress = (size_t)ucHeap;
	if ((uxAddress & portBYTE_ALIGNMENT_MASK) != 0)
	{
		uxAddress += (portBYTE_ALIGNMENT - 1);
		uxAddress &= ~((size_t)portBYTE_ALIGNMENT_MASK);
		xTotalHeapSize -= uxAddress - (size_t)ucHeap;
	}
	pucAlignedHeap = (uint8_t *)uxAddress;

	xStart.pxNextFreeBlock = (void *)pucAlignedHeap;
	xStart.xBlockSize = (size_t)0;

	uxAddress = ((size_t)pucAlignedHeap) + xTotalHeapSize;
	uxAddress -= xHeapStructSize;
	uxAddress &= ~((size_t)portBYTE_ALIGNMENT_MASK);
	pxEnd = (void *)uxAddress;
	pxEnd->xBlockSize = 0;
	pxEnd->pxNextFreeBlock = NULL;

	pxFirstFreeBlock = (void *)pucAlignedHeap; /*！！！使用局部指针初始化全局内存*/
	pxFirstFreeBlock->xBlockSize = uxAddress - (size_t)pxFirstFreeBlock;
	pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

	xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
	xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;

	xBlockAllocatedBit = ((size_t)1) << ((sizeof(size_t) * (size_t)8) - 1);
}

/**
 * @brief 将释放的内存块插入到空闲内存块链表中的正确位置。如果内存块彼此相邻，则进行合并
 */
static void prvInsertBlockIntoFreeList(BlockLink_t *pxBlockToInsert)
{
	BlockLink_t *pxIterator;
	uint8_t *puc;

	for (pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock)
	{
	}

	puc = (uint8_t *)pxIterator;
	if ((puc + pxIterator->xBlockSize) == (uint8_t *)pxBlockToInsert)
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
	}

	puc = (uint8_t *)pxBlockToInsert;
	if ((puc + pxBlockToInsert->xBlockSize) == (uint8_t *)pxIterator->pxNextFreeBlock)
	{
		if (pxIterator->pxNextFreeBlock != pxEnd)
		{
			pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
			pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
		}
		else
		{
			pxBlockToInsert->pxNextFreeBlock = pxEnd;
		}
	}
	else
	{
		pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
	}

	if (pxIterator != pxBlockToInsert)
	{
		pxIterator->pxNextFreeBlock = pxBlockToInsert;
	}
}

void *pvPortMalloc(size_t xWantedSize)
{
	BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
	void *pvReturn = NULL;

	if (pxEnd == NULL)
	{
		prvHeapInit();
	}

	if ((xWantedSize & xBlockAllocatedBit) == 0)
	{
		/* The wanted size is increased so it can contain a BlockLink_t
		structure in addition to the requested amount of bytes. */
		if (xWantedSize > 0)
		{
			xWantedSize += xHeapStructSize;

			if ((xWantedSize & portBYTE_ALIGNMENT_MASK) != 0x00)
			{
				xWantedSize += (portBYTE_ALIGNMENT - (xWantedSize & portBYTE_ALIGNMENT_MASK));
			}
		}

		if ((xWantedSize > 0) && (xWantedSize <= xFreeBytesRemaining))
		{
			pxPreviousBlock = &xStart;
			pxBlock = xStart.pxNextFreeBlock;
			while ((pxBlock->xBlockSize < xWantedSize) && (pxBlock->pxNextFreeBlock != NULL))
			{
				pxPreviousBlock = pxBlock;
				pxBlock = pxBlock->pxNextFreeBlock;
			}

			if (pxBlock != pxEnd)
			{
				pvReturn = (void *)(((uint8_t *)pxPreviousBlock->pxNextFreeBlock) + xHeapStructSize);

				pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

				if ((pxBlock->xBlockSize - xWantedSize) > heapMINIMUM_BLOCK_SIZE)
				{
					pxNewBlockLink = (void *)(((uint8_t *)pxBlock) + xWantedSize);

					pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
					pxBlock->xBlockSize = xWantedSize;

					prvInsertBlockIntoFreeList(pxNewBlockLink);
				}

				xFreeBytesRemaining -= pxBlock->xBlockSize;

				if (xFreeBytesRemaining < xMinimumEverFreeBytesRemaining)
				{
					xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
				}

				pxBlock->xBlockSize |= xBlockAllocatedBit;
				pxBlock->pxNextFreeBlock = NULL;
			}
		}
	}

	return pvReturn;
}

void vPortFree(void *pv)
{
	uint8_t *puc = (uint8_t *)pv;
	BlockLink_t *pxLink;

	if (pv != NULL)
	{
		puc -= xHeapStructSize;

		pxLink = (void *)puc;

		if ((pxLink->xBlockSize & xBlockAllocatedBit) != 0)
		{
			if (pxLink->pxNextFreeBlock == NULL)
			{
				pxLink->xBlockSize &= ~xBlockAllocatedBit;

				/* Add this block to the list of free blocks. */
				xFreeBytesRemaining += pxLink->xBlockSize;
				prvInsertBlockIntoFreeList(((BlockLink_t *)pxLink));
			}
		}
	}
}

size_t xPortGetFreeHeapSize( void )
{
	return xFreeBytesRemaining;
}

size_t xPortGetMinimumEverFreeHeapSize( void )
{
	return xMinimumEverFreeBytesRemaining;
}
