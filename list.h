#ifndef __LIST_H
#define __LIST_H

#ifdef THREAD_SAFE
#include <pthread.h>
#endif

/* Define the list item */
struct _list_item {
	void *item;
	int size;
	struct _list_item *prev;
	struct _list_item *next;
};
typedef struct _list_item * list_item;
#define LIST_ITEM_SIZE sizeof(struct _list_item)

/* Define the list */
struct _list {
	int type;			/* Data type in list */
	list_item first;		/* First item in list */
	list_item last;			/* Last item in list */
	list_item next;			/* Next item in list */
#ifdef THREAD_SAFE
	pthread_mutex_t mutex;
#endif
};
typedef struct _list * list;
#define LIST_SIZE sizeof(struct _list)

#ifdef __cplusplus
extern "C" {
#endif
/* Define the list functions */
list list_create( void );
int list_destroy( list );
list list_dup( list );
void *list_add( list, void *, int );
int list_add_list( list, list );
int list_delete( list, void * );
void *list_get_next( list );
#define list_next list_get_next
int list_reset( list );
int list_count( list );
int list_is_next( list );
typedef int (*list_compare)(list_item, list_item);
int list_sort( list, list_compare, int);
#ifdef __cplusplus
}
#endif

#endif /* __LIST_H */
