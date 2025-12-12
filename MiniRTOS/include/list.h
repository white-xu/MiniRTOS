#ifndef LIST_H
#define LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MiniRTOS.h"

/**
 * @brief 链表项定义
 * @note  There is therefore a two way link between the object containing the list item and the list item itself. 
 */
struct xLIST_ITEM
{
	TickType_t xItemValue;			/*链表项的值，大多数情况下用于对链表进行降序排列*/
	struct xLIST_ITEM *pxNext;		/*指向链表中的下一个链表项*/
	struct xLIST_ITEM *pxPrevious;	/*指向链表中的前一个链表项*/
	void *pvOwner;					/*指向包含此链表项的对象(通常是任务控制块TCB)*/
	void *pvContainer;				/*指向包含此链表项的链表*/
};
typedef struct xLIST_ITEM ListItem_t;

struct xMINI_LIST_ITEM
{
	TickType_t xItemValue;
	struct xLIST_ITEM *pxNext;
	struct xLIST_ITEM *pxPrevious;
};
typedef struct xMINI_LIST_ITEM MiniListItem_t;

/**
 * @brief 链表定义
 */
typedef struct xLIST
{
	UBaseType_t uxNumberOfItems;    /*链表项的数量*/
	ListItem_t *pxIndex;			/*用于遍历链表，listGET_OWNER_OF_NEXT_ENTRY会使它指向下一个链表项*/
	MiniListItem_t xListEnd;		/*包含最大可能值的列表项，这意味着它总是位于列表的末尾，因此被用作标记项。为节省空间使用Mini链表项*/
} List_t;

/**
 * @brief 设置链表项的所有者。所有者是指包含链表项的对象(通常是TCB)
 */
#define listSET_LIST_ITEM_OWNER( pxListItem, pxOwner )		( ( pxListItem )->pvOwner = ( void * ) ( pxOwner ) )

/**
 * @brief 返回链表项的所有者
 */
#define listGET_LIST_ITEM_OWNER( pxListItem )	( ( pxListItem )->pvOwner )

/**
 * @brief 设置链表项的值。在大多数情况下，该值用来对链表进行降序排序
 */
#define listSET_LIST_ITEM_VALUE( pxListItem, xValue )	( ( pxListItem )->xItemValue = ( xValue ) )

/**
 * @brief 返回链表项的值
 */
#define listGET_LIST_ITEM_VALUE( pxListItem )	( ( pxListItem )->xItemValue )

/**
 * @brief 返回给定链表头部链表项的值
 */
#define listGET_ITEM_VALUE_OF_HEAD_ENTRY( pxList )	( ( ( pxList )->xListEnd ).pxNext->xItemValue )

/**
 * @brief 返回链表头部的链表项
 */
#define listGET_HEAD_ENTRY( pxList )	( ( ( pxList )->xListEnd ).pxNext )

/**
 * @brief 返回给定链表项指向的下一个链表项
 */
#define listGET_NEXT( pxListItem )	( ( pxListItem )->pxNext )

/**
 * @brief 返回链表末尾的链表项
 */
#define listGET_END_MARKER( pxList )	( ( ListItem_t const * ) ( &( ( pxList )->xListEnd ) ) )

/**
 * @brief 判断链表是否为空。链表为空时，该宏的值为真
 */
#define listLIST_IS_EMPTY( pxList )	( ( BaseType_t ) ( ( pxList )->uxNumberOfItems == ( UBaseType_t ) 0 ) )

/**
 * @brief 返回当前链表长度
 */
#define listCURRENT_LIST_LENGTH( pxList )	( ( pxList )->uxNumberOfItems )

/**
 * @brief 获取链表中下一个链表项的所有者
 * @note  链表的成员pxIndex用于遍历链表，调用listGET_OWNER_OF_NEXT_ENTRY递增pxIndex
 * 		  使其指向链表的下一个链表项，并返回该链表项的pxOwner字段。通过多次调用该宏，就
 * 		  可以遍历链表中包含的每一项。
 */
#define listGET_OWNER_OF_NEXT_ENTRY( pxTCB, pxList )										\
{																							\
List_t * const pxConstList = ( pxList );													\
	/* Increment the index to the next item and return the item, ensuring */				\
	/* we don't return the marker used at the end of the list.  */							\
	( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext;							\
	if( ( void * ) ( pxConstList )->pxIndex == ( void * ) &( ( pxConstList )->xListEnd ) )	\
	{																						\
		( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext;						\
	}																						\
	( pxTCB ) = ( pxConstList )->pxIndex->pvOwner;											\
}

/**
 * @brief 返回链表的头链表项的所有者
 */
#define listGET_OWNER_OF_HEAD_ENTRY( pxList )  ( (&( ( pxList )->xListEnd ))->pxNext->pvOwner )

/**
 * @brief 判断链表项是否在给定的链表中。包含时，该宏的值为真
 */
#define listIS_CONTAINED_WITHIN( pxList, pxListItem ) ( ( BaseType_t ) ( ( pxListItem )->pvContainer == ( void * ) ( pxList ) ) )

/**
 * @brief 返回包含该链表项的链表
 */
#define listLIST_ITEM_CONTAINER( pxListItem ) ( ( pxListItem )->pvContainer )

/**
 * @brief 判断一个链表是否已初始化
 * @note  如果一个链表已经初始化，vListInitialise()函数会将
 * 		  pxList->xListEnd.xItemValue字段设置为portMAX_DELAY
 */
#define listLIST_IS_INITIALISED( pxList ) ( ( pxList )->xListEnd.xItemValue == portMAX_DELAY )


void vListInitialise( List_t * const pxList );
void vListInitialiseItem( ListItem_t * const pxItem );
void vListInsert( List_t * const pxList, ListItem_t * const pxNewListItem );
void vListInsertEnd( List_t * const pxList, ListItem_t * const pxNewListItem );
UBaseType_t uxListRemove( ListItem_t * const pxItemToRemove );

#ifdef __cplusplus
}
#endif

#endif

