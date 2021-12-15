// Simple C callback list
// JCE, 9-11-2020

#include <stdlib.h>

typedef struct s_callback_list
{
	struct s_callback_list *next;
	void (*func)(void*);
	void* par;
} callback_list;

// Add a callback to the list
void cb_add(callback_list **cbl, void (*func)(void*), void *par)
{
	callback_list *item = malloc(sizeof(callback_list));
	if (!item)
		return;
	item->next = *cbl;
	item->func = func;
	item->par = par;
	*cbl = item;
}

// Free the whole callback list from memory
void cb_free(callback_list **cblp)
{
	callback_list *cbl = *cblp;
	*cblp = NULL;
	callback_list *next;
	while(cbl)
	{
		next = cbl->next;
		free(cbl);
		cbl = next;
	}
}

// Call all callbacks in the list
void cb_call(callback_list *cbl)
{
	while(cbl)
	{
		cbl->func(cbl->par);
		cbl = cbl->next;
	}
}
