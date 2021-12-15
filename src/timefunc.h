// Timefunctions as they seem logical to me.
// JCE, 24-2-2021

#ifndef TIMEFUNC
#define TIMEFUNC

#ifdef __cplusplus
extern "C" {
#endif

// Reads the monotonic clock.
double get_time_monotonic();

// Write a unix timestamp (not the monotonic time) as human readable string.
// Supply a write buffer of at least 27 characters length.
void write_human_time(char*, double);

// Read a human readable time, formatted as dd-mm-yyyy hh:mm:ss, to unix timestamp.
// Returns 1 on success, 0 on failure.
int read_human_time(const char*, double*);

#ifdef __cplusplus
}
#endif

#endif // TIMEFUNC
