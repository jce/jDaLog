#include "stdio.h"
#include "sys/time.h" // gettimeofday(()

using namespace std;

double now(){
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + (double) tv.tv_usec / 1000000;}

