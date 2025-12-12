#include "list.h"

/**
 * @brief 初始化链表。此函数会初始化链表结构中的所有成员，并将xListEnd项插入到链表中
 *        作为链表尾部的标记
 */
void vListInitialise( List_t * const pxList )
{
    pxList->pxIndex = ( ListItem_t * ) &( pxList->xListEnd );

    pxList->xListEnd.xItemValue = portMAX_DELAY;

    pxList->xListEnd.pxNext = ( ListItem_t * ) &( pxList->xListEnd );
	pxList->xListEnd.pxPrevious = ( ListItem_t * ) &( pxList->xListEnd );

    pxList->uxNumberOfItems = ( UBaseType_t ) 0U;
}

/**
 * @brief 初始化链表项。该函数将pvContainer字段设置为NULL，从而避免该项认为自己包含
 *        在链表中
 */
void vListInitialiseItem( ListItem_t * const pxItem )
{
    pxItem->pvContainer = NULL;
}

/**
 * @brief 将一个链表项插入到链表中。根据链表项的值按降序在链表中进行插入
 */
void vListInsert( List_t * const pxList, ListItem_t * const pxNewListItem )
{
    ListItem_t *pxIterator;
    const TickType_t xValueOfInsertion = pxNewListItem->xItemValue;

    if( xValueOfInsertion == portMAX_DELAY )
		pxIterator = pxList->xListEnd.pxPrevious;
    else
        for( pxIterator = ( ListItem_t * ) &( pxList->xListEnd ); pxIterator->pxNext->xItemValue <= xValueOfInsertion; pxIterator = pxIterator->pxNext )
        {
            
        }

    pxNewListItem->pxNext = pxIterator->pxNext;
	pxNewListItem->pxNext->pxPrevious = pxNewListItem;
	pxNewListItem->pxPrevious = pxIterator;
	pxIterator->pxNext = pxNewListItem;

    pxNewListItem->pvContainer = ( void * ) pxList;

    ( pxList->uxNumberOfItems )++;
}

/**
 * @brief 将一个链表项插入到链表中。该项将被插入到特定位置————通过多次调用
 *        listGET_OWNER_OF_NEXT_ENTRY宏所返回的链表中的最后一项
 */
void vListInsertEnd( List_t * const pxList, ListItem_t * const pxNewListItem )
{
    ListItem_t * const pxIndex = pxList->pxIndex;

    pxNewListItem->pxNext = pxIndex;
	pxNewListItem->pxPrevious = pxIndex->pxPrevious;

    pxIndex->pxPrevious->pxNext = pxNewListItem;
	pxIndex->pxPrevious = pxNewListItem;

    pxNewListItem->pvContainer = ( void * ) pxList;

	( pxList->uxNumberOfItems )++;
}

/**
 * @brief 从链表中移除链表项
 */
UBaseType_t uxListRemove( ListItem_t * const pxItemToRemove )
{
    List_t * const pxList = ( List_t * ) pxItemToRemove->pvContainer;

    pxItemToRemove->pxNext->pxPrevious = pxItemToRemove->pxPrevious;
	pxItemToRemove->pxPrevious->pxNext = pxItemToRemove->pxNext;

    if( pxList->pxIndex == pxItemToRemove )
		pxList->pxIndex = pxItemToRemove->pxPrevious;

    pxItemToRemove->pvContainer = NULL;
	( pxList->uxNumberOfItems )--;

	return pxList->uxNumberOfItems;
}
