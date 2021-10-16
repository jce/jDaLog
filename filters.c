#include "filters.h"

#include "stdlib.h"
#include "string.h"

typedef struct blockfilter_t
{
	uint32_t len;		// Length of array values.
	uint32_t used;		// Used spots in values array.
	uint32_t index;		// Index for next value to be written
	uint64_t sum;		// Sum of all values in the array
	uint32_t val[];		// Array with values
} blockfilter;

blockfilter* new_blockfilter(uint32_t len)
{
	size_t size = sizeof(blockfilter) + sizeof(uint32_t) * len;
	blockfilter *bf = malloc(size);
	if(bf)
	{
		memset(bf, 0, size);
		bf->len = len;
	}
	return bf;
}

void blockfilter_clear(blockfilter *bf)
{
	bf->used = 0;
	bf->index = 0;
	bf->sum = 0;
	for (uint32_t i = 0; i < bf->len; i++)
		bf->val[i] = 0;
}

int blockfilter_filled(const blockfilter *bf)
{
	return bf->used == bf->len;
}

void blockfilter_new_val(blockfilter *bf, uint32_t val)
{
	bf->sum -= bf ->val[ bf->index ];
	bf->val[ bf->index ] = val;
	bf->sum += val;
	bf->index = (bf->index + 1) % bf->len;
	if (bf->used < bf->len)
		bf->used++;
}

float blockfilter_get_average(const blockfilter *bf)
{
	float rv = 0;
	if (bf->used)
		rv = (float) bf->sum / bf->used;
	return rv;
}

