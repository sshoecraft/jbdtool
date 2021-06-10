
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "list.h"

#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG 1

#ifdef DEBUG
#undef DPRINTF
#define DPRINTF(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#else
#define DPRINTF(format, args...) /* noop */
#endif

#define _dump(i) DPRINTF("%s: item: %p, prev: %p, next: %p\n", #i, i, i->prev, i->next);
void list_checkitem(list_item ip1) {
	_dump(ip1);
	assert(ip1->prev != ip1);
	assert(ip1->next != ip1);
	if (ip1->prev) {
		_dump(ip1->prev);
		assert(ip1->prev->prev != ip1);
		assert(ip1->prev->next == ip1);
	}
	if (ip1->next) {
		_dump(ip1->next);
		assert(ip1->next->prev == ip1);
		assert(ip1->next->next != ip1);
	}
}

static list_item _newitem(void *item, int size) {
	list_item new_item;

#if DEBUG
	printf("_newitem: item: %p, size: %d\n", item, size);
#endif
	new_item = (list_item) malloc(LIST_ITEM_SIZE);
	if (!new_item) return 0;

	if (size) {
		new_item->item = (void *) malloc((size_t)size);
		if (!new_item->item) {
			free(new_item);
#if DEBUG
			printf("_newitem: returning: 0");
#endif
			return 0;
		}
		memcpy(new_item->item,item,size);
	} else {
		new_item->item = item;
	}

	new_item->size = size;
	new_item->prev = (list_item) 0;
	new_item->next = (list_item) 0;

#if DEBUG
	printf("_newitem: returning: %p\n", new_item);
#endif
	return new_item;
}

void *list_add(list lp, void *item, int size) {
	list_item ip, new_item;

#if DEBUG
	printf("\n\n*********************\n");
	printf("list_add: lp: %p, item: %p, size: %d\n", lp, item, size);
#endif
	if (!lp) return 0;

	/* Create a new item */
	new_item = _newitem(item,size);
	if (!new_item) return 0;

#ifdef THREAD_SAFE
	pthread_mutex_lock(&lp->mutex);
#endif
	/* Add it to the list */
#if DEBUG
	printf("ip->first: %p\n", lp->first);
#endif
	if (!lp->first)
		lp->first = lp->last = lp->next = new_item;
	else {
		ip = lp->last;                  /* Get last item */
		ip->next = new_item;            /* Add this one */
		new_item->prev = ip;		/* Point to it */
		lp->last = new_item;            /* Make this last */
	}
#ifdef THREAD_SAFE
	pthread_mutex_unlock(&lp->mutex);
#endif
	return new_item->item;
}

int list_add_list(list lp,list lp2) {
	list_item ip;

	if (!lp) return 1;

#ifdef THREAD_SAFE
	pthread_mutex_lock(&lp->mutex);
	pthread_mutex_lock(&lp2->mutex);
#endif

	/* Get each item from the old list and copy it to the new one */
	ip = lp2->first;
	while(ip) {
		list_add(lp,ip->item,ip->size);
		ip = ip->next;
	}

#ifdef THREAD_SAFE
	pthread_mutex_unlock(&lp->mutex);
	pthread_mutex_unlock(&lp2->mutex);
#endif

	/* Return the new list */
	return 0;
}

int list_count(list lp) {
	list_item ip;
	register int count;

	if (!lp) return -1;

#ifdef THREAD_SAFE
	pthread_mutex_lock(&lp->mutex);
#endif
	count = 0;
	for(ip = lp->first; ip; ip = ip->next) count++;

#ifdef THREAD_SAFE
	pthread_mutex_unlock(&lp->mutex);
#endif
	return count;
}

list list_create(void) {
	list lp;

	lp = (list) malloc(LIST_SIZE);
#if DEBUG
	printf("list_create: lp: %p\n", lp);
#endif
	if (!lp) return 0;

	lp->first = lp->last = lp->next = (list_item) 0;

#ifdef THREAD_SAFE
	pthread_mutex_init(&lp->mutex,NULL);
#endif

#if DEBUG
	printf("list_create: returning: %p\n", lp);
#endif
	return lp;
}

int list_delete(list lp,void *item) {
	list_item ip,prev,next;

	if (!lp) return -1;

#ifdef THREAD_SAFE
	pthread_mutex_lock(&lp->mutex);
#endif

	ip = lp->first;
	while(ip) {
		if (item == ip->item) {
			prev = ip->prev;        /* Get the pointers */
			next = ip->next;

			/* Fixup links in other first */
			if (prev) prev->next = next;
			if (next) next->prev = prev;

			/* Was this the 1st item? */
			if (ip == lp->first) lp->first = next;

			/* Was this the last item? */
			if (ip == lp->last) lp->last = prev;

			/* Was this the next item? */
			if (ip == lp->next) lp->next = next;

			free(item);		/* Free the item */
			free(ip);		/* Free the ptr */
#ifdef THREAD_SAFE
			pthread_mutex_unlock(&lp->mutex);
#endif
			return 0;		/* Item found and deleted */
		}
		ip = ip->next;                  /* Next item, please. */
	}
#ifdef THREAD_SAFE
	pthread_mutex_unlock(&lp->mutex);
#endif
	return 1;                               /* Item not found, error. */
}

int list_destroy(list lp) {
	list_item ip,next;

	if (!lp) return -1;

	ip = lp->first;                         /* Start at beginning */
	while(ip) {
		if (ip->size) free(ip->item);	/* Free the item */
		next = ip->next;                /* Get next pointer */
		free(ip);			/* Free current item */
		ip = next;                      /* Set current item to next */
	}
	free(lp);				/* Free list */
	return 0;
}

list list_dup(list lp) {
	list new_list;
	list_item ip;

	if (!lp) return 0;

	/* Create a new list */
	new_list = list_create();
	if (!new_list) return 0;

#ifdef THREAD_SAFE
	pthread_mutex_lock(&lp->mutex);
#endif

	/* Get each item from the old list and copy it to the new one */
	ip = lp->first;
	while(ip) {
		list_add(new_list,ip->item,ip->size);
		ip = ip->next;
	}

#ifdef THREAD_SAFE
	pthread_mutex_unlock(&lp->mutex);
#endif

	/* Return the new list */
	return new_list;
}

void *list_get_next(list lp) {
	list_item ip;
	void *item;

	if (!lp) return 0;

#ifdef THREAD_SAFE
	pthread_mutex_lock(&lp->mutex);
#endif

	item = 0;
	if (lp->next) {
		ip = lp->next;
		lp->next = ip->next;
		item = ip->item;
	}

#ifdef THREAD_SAFE
	pthread_mutex_unlock(&lp->mutex);
#endif
	return item;
}

int list_is_next(list lp) {
	if (!lp) return -1;
	return(lp->next ? 1 : 0);
}

int list_reset(list lp) {

	if (!lp) return -1;

	lp->next = lp->first;
	return 0;
}

static int _compare(list_item item1, list_item item2) {
	register int val;

	val = strcmp(item1->item,item2->item);
	if (val < 0)
		val =  -1;
	else if (val > 0)
		val = 1;
	return val;
}


int list_sort(list lp, list_compare compare, int order) {
	int comp,swap,save_size;
	list_item save_item;
	register list_item ip1,ip2;

	if (!lp) return -1;

#ifdef THREAD_SAFE
	pthread_mutex_lock(&lp->mutex);
#endif

	/* If compare is null, use internal compare (strcmp) */
	if (!compare) {
		compare = _compare;
	}

	/* Set comp negative or positive, depending upon order */
	comp = (order == 0 ? 1 : -1);

	/* Sort the list */
	for(ip1 = lp->first; ip1;) {
		swap = 0;
		for(ip2 = ip1->next; ip2; ip2 = ip2->next) {
			if (compare(ip1,ip2) == comp) {
				swap = 1;
				break;
			}
		}
		DPRINTF("swap: %d\n", swap);
		if (swap) {
			/* LOL dont swap the list_items ... just swap the items */
			save_item = ip1->item;
			save_size = ip1->size;
			ip1->item = ip2->item;
			ip1->size = ip2->size;
			ip2->item = save_item;
			ip2->size = save_size;
		} else
			ip1 = ip1->next;
	}
#ifdef THREAD_SAFE
	pthread_mutex_unlock(&lp->mutex);
#endif
	return 0;
}
