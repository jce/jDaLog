#include "timefunc.h"

#include "time.h"

// Timefunctions as they seem logical to me.
// JCE, 24-2-2021


double get_time_monotonic()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);	
	return ts.tv_sec + (double) ts.tv_nsec / 1000000000;
}


