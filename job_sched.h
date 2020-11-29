#ifndef JOB_SCHED_H
#define JOB_SCHED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


// Job scheduler.

// It consists of a number of threads, that perform jobs that can be given to them.
// Primarily intended for handling scripts at triggering events while not delaying the primary
// program logic. Quite some inspiration has been obtained from https://github.com/Pithikos/C-Thread-Pool
// but this would not be exactly what is needed.
// JCE, 22-10-2019
// Targets for version 1:
// - Fixed number of threads
// - No timing, only "perform this job right now"
// - No repeating constructs
// - Lua interpreter (not in scope of this threadpool, but still waypoint for this version)
// - In C because, i hate classes. And it is supposed to be simple and a building block for the rest of the program.

// Allocation size for job items.
#define JOS_BLOCK_SIZE		64
#define JOS_CLOCK_SOURCE	CLOCK_MONOTONIC_COARSE

// Opaque type for the host program: one pool. Each job scheduler operation requires a pool to perform on. 
struct jos_pool;

// tp_new_pool returns a threadpool of the specified number of threads or NULL if
// for some reason construction did not succeed. It will never return a smaller pool.
struct jos_pool* jos_new_pool(unsigned int num_threads);

// Obvious, this deletes a pool. It waits for all jobs to be completed, then cleans up.
void jos_delete_pool(struct jos_pool *pool);

// Give a function to the pool to be run.
// 1 - The pool (this is C, not C++ heh )
// 2 - The function (function pointer to function with one void* argument that returns nothing)
// 3 - The void* argument that is given to the function
// Returns int. 0 = Success, 1 = Failed (job memory allocation failed), 2 = Failed (Pool pointer was NULL)
int jos_run(struct jos_pool*, void (*)(void*), void*);

// Get the number of pending tasks
//uint32_t tp_get_num_tasks(struct tp_pool *pool);

// Get the number of pending jobs
//uint32_t tp_get_num_jobs(struct tp_pool *pool);


#ifdef __cplusplus
}
#endif

#endif /* JOS_SCHED_H */

