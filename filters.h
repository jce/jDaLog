#ifndef HAVE_FILTERS_H
#define HAVE_FILTERS_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

// FIR average over samples

typedef struct blockfilter_t blockfilter;

blockfilter* new_blockfilter(uint32_t len);
void blockfilter_clear(blockfilter*);
int blockfilter_filled(const blockfilter*);
void blockfilter_new_val(blockfilter*, uint32_t);
float blockfilter_get_average(const blockfilter*);

#ifdef __cplusplus
}
#endif

#endif // HAVE_FILTERS_H
