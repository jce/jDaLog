#include "timefunc.h"

#include "stdio.h"
#include "time.h"

// Timefunctions as they seem logical to me.
// JCE, 24-2-2021


double get_time_monotonic()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);	
	return ts.tv_sec + (double) ts.tv_nsec / 1000000000;
}

// Write a unix timestamp (not the monotonic time) as human readable string.
void write_human_time(char *buf, double time)
{
	struct timespec ts;
	struct tm t;
	ts.tv_sec = time;
	ts.tv_nsec = (time - ts.tv_sec) * 1000000000;
	localtime_r(&ts.tv_sec, &t);

	sprintf(buf, "%02d-%02d-%04d %02d:%02d:%09f", t.tm_mday, t.tm_mon+1, t.tm_year+1900, t.tm_hour, t.tm_min, (double) t.tm_sec + (double) ts.tv_nsec / 1000000000);
}

// Read a human readable time, formatted as dd-mm-yyyy hh:mm:ss, to unix timestamp.
// Returns 1 on success, 0 on failure.
int read_human_time(const char *buf, double *time)
{
	double sec;
	struct timespec ts;
	struct tm t;
	if ( sscanf(buf, "%d-%d-%d %d:%d:%lf", &t.tm_mday, &t.tm_mon, &t.tm_year, &t.tm_hour, &t.tm_min, &sec) != 6)
		return 0;
	t.tm_mon -= 1;
	t.tm_year -= 1900;
	t.tm_sec = sec;
	ts.tv_nsec = (sec - t.tm_sec) * 1000000000;
	ts.tv_sec = mktime(&t);
	*time = ts.tv_sec + ts.tv_nsec / 1000000000;	
	return 1;
}
