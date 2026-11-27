// Simple C callback list
// JCE, 9-11-2020
//
// Threading: every list owns a mutex (in its head). Mutating a list (cb_add /
// cb_remove / cb_free) is intended for startup/shutdown only. cb_call may run
// concurrently on different lists without blocking each other; on one list it
// snapshots the subscribers under the mutex and invokes them after unlocking,
// so a callback may safely call back into this API (no deadlock, no walking a
// list that is being mutated).

#include <stdlib.h>
#include <pthread.h>

#include "callback.h"

typedef struct s_callback_node
{
	struct s_callback_node *next;
	void (*func)(void*);
	void* par;
} callback_node;

struct callback_list
{
	pthread_mutex_t mutex;		// guards first
	callback_node *first;
};

// Add a callback to the list
void cb_add(callback_list **cblp, void (*func)(void*), void *par)
{
	if (!cblp)
		return;
	if (!*cblp)
	{
		callback_list *h = malloc(sizeof(callback_list));
		if (!h)
			return;
		pthread_mutex_init(&h->mutex, NULL);
		h->first = NULL;
		*cblp = h;
	}
	callback_list *h = *cblp;
	pthread_mutex_lock(&h->mutex);
	// Skip duplicates: registering the same function with the same argument twice
	// would make it fire twice (e.g. when out_conf is re-run).
	for (callback_node *i = h->first; i; i = i->next)
		if (i->func == func && i->par == par)
		{
			pthread_mutex_unlock(&h->mutex);
			return;
		}
	callback_node *item = malloc(sizeof(callback_node));
	if (item)
	{
		item->next = h->first;
		item->func = func;
		item->par = par;
		h->first = item;
	}
	pthread_mutex_unlock(&h->mutex);
}

// Remove all callbacks matching func and par from the list
void cb_remove(callback_list **cblp, void (*func)(void*), void *par)
{
	if (!cblp || !*cblp)
		return;
	callback_list *h = *cblp;
	pthread_mutex_lock(&h->mutex);
	callback_node **i = &h->first;
	while (*i)
	{
		if ((*i)->func == func && (*i)->par == par)
		{
			callback_node *del = *i;
			*i = del->next;
			free(del);
		}
		else
			i = & (*i)->next;
	}
	pthread_mutex_unlock(&h->mutex);
}

// Free the whole callback list from memory
void cb_free(callback_list **cblp)
{
	if (!cblp || !*cblp)
		return;
	callback_list *h = *cblp;
	pthread_mutex_lock(&h->mutex);
	callback_node *n = h->first;
	while (n)
	{
		callback_node *next = n->next;
		free(n);
		n = next;
	}
	h->first = NULL;
	pthread_mutex_unlock(&h->mutex);
	pthread_mutex_destroy(&h->mutex);
	free(h);
	*cblp = NULL;
}

// Call all callbacks in the list
void cb_call(callback_list *cbl)
{
	if (!cbl)
		return;
	pthread_mutex_lock(&cbl->mutex);
	size_t n = 0;
	for (callback_node *i = cbl->first; i; i = i->next)
		n++;
	if (!n)
	{
		pthread_mutex_unlock(&cbl->mutex);
		return;
	}
	// Snapshot the subscribers so the callbacks run without the mutex held.
	callback_node *snap = malloc(n * sizeof(callback_node));
	if (!snap)
	{
		// OOM: fall back to calling under the lock (mutation is startup/shutdown only).
		for (callback_node *i = cbl->first; i; i = i->next)
			i->func(i->par);
		pthread_mutex_unlock(&cbl->mutex);
		return;
	}
	size_t k = 0;
	for (callback_node *i = cbl->first; i; i = i->next)
	{
		snap[k].func = i->func;
		snap[k].par = i->par;
		k++;
	}
	pthread_mutex_unlock(&cbl->mutex);
	for (k = 0; k < n; k++)
		snap[k].func(snap[k].par);
	free(snap);
}
