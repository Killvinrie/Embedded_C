``#define listGET_OWNER_OF_NEXT_ENTRY( pxTCB, pxList )

```c
do { \

	List_t * const pxConstList = ( pxList ); \

/* Increment the index to the next item and return the item, ensuring */ \

/* we don't return the marker used at the end of the list. */ \

	( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext; \

	if( ( void * ) ( pxConstList )->pxIndex == ( void * ) &( ( pxConstList )->xListEnd ) ) \

	{ \

		( pxConstList )->pxIndex = ( pxConstList )->xListEnd.pxNext; \

	} \

	( pxTCB ) = ( pxConstList )->pxIndex->pvOwner; \

} while( 0 )

```

