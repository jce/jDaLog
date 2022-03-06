#ifndef JOB_SCHED_H
#define JOB_SCHED_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


// Job scheduler.

// Allocation size for job items.
#define JOS_BLOCK_SIZE		64

// Clock source. Coarse monotonic recommended. See man clock_gettime for options.
//#define JOS_CLOCK_SOURCE	CLOCK_MONOTONIC_COARSE 	// pthread_cond_timedwait seems to not wait on this ever.
#define JOS_CLOCK_SOURCE	CLOCK_MONOTONIC			// works fine.
//#define JOS_CLOCK_SOURCE	CLOCK_REALTIME_COARSE	// Works, but returns based on the normal clock? COARSE time is a bit behind, so like 200 wakeups result
//#define JOS_CLOCK_SOURCE	CLOCK_REALTIME			// Works. Obviously not the best choice.

// Opaque type for the host program: one pool. Each job scheduler operation requires a pool to perform on. 
typedef struct jos_pool jos_pool;

// tp_new_pool returns a threadpool of the specified number of threads or NULL if
// for some reason construction did not succeed. It will never return a smaller pool.
struct jos_pool* jos_new_pool(unsigned int num_threads);

// Delete a pool. It waits for all jobs to be completed, then cleans up.
void jos_delete_pool(jos_pool *pool);

// Give a function to the pool to be run right away.
// 1 - The pool
// 2 - The function (function pointer to function with one void* argument that returns nothing)
// 3 - The void* argument that is given to the function
// Returns int. 0 = Success, 1 = Failed (job memory allocation failed), 2 = Failed (Pool pointer was NULL)
void jos_run(jos_pool*, void (*)(void*), void*);

// Run at a specific timestamp
//void jos_run_at(struct jos_pool*, const struct timespec*, void (*)(void*), void*);

// Run after a given time [s]
void jos_run_in(jos_pool*, float, void (*)(void*), void*);

// Run at a given time [s] on the JOS_CLOCK_SOURCE clock.
void jos_run_at(jos_pool*, double, void (*)(void*), void*);

// Run repeatedly every given interval [s]
void jos_run_every(jos_pool*, double, void (*)(void*), void*);

// Remove all scheduled actions for a function
// void jos_remove_fnc(jos_pool*, void (*)(void*));

// Remove all scheduled actions for a function with parameter
void jos_remove(jos_pool*, void (*)(void*), void*);

// Debug
void jos_print(jos_pool*);
size_t jos_printn(jos_pool*, char*, size_t);

#ifdef __cplusplus

// Template to create a trampline function in a C++ class.
// Creates a C Callable function void <_CLASS_>::cc_<_FNC_>(void*) which will in turn call
// _FNC_ of a given instance of _CLASS_. Register to jos with
// jos_run(pool, class::fnc, (void*) this);
#define CC(_CLASS_, _FNC_) static void cc_##_CLASS_##_##_FNC_(void* p) { ((_CLASS_*) p) -> _FNC_(); }

}
#endif

#endif /* JOB_SCHED_H */

