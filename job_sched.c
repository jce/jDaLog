// JOb Scheduler (JOS for short):
// - Manages a job pool of fixed size
// - Can be given jobs to be executed now or in the future
// - Preserves job order
// JCE, 29-11-2020

// Quite some inspiration has been obtained from https://github.com/Pithikos/C-Thread-Pool
// but this would not be exactly what is needed.
// JCE, 22-10-2019

#include "job_sched.h"

#include <pthread.h>	// mutex, condition variable
#include <stdlib.h>		// malloc()
#include <time.h>		// clock source macro's
#include <unistd.h>		// sleep

#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include <stdio.h>
#include <time.h>

// Individually scheduled job item.
typedef struct jos_job
{
	struct jos_job *next;
	struct timespec at;
	void (*fnc)(void*);
	void* arg;
} jos_job;

// Block of scheduled job items, mainly for free.
typedef struct jos_job_block
{
	struct jos_job_block *next;
	jos_job job[];
} jos_job_block;

// Individual thread handling jobs.
typedef struct jos_thread
{
	pthread_t thread;					// Holds the thread itself.
	volatile unsigned int working;		// Contains 1 if working, 0 if idle. Can be read without mutex, not allowed to be written.
} jos_thread;

typedef struct jos_pool
{
	int run;					// Flag to keep threads running. 1 = run, 0 = stop.
	pthread_mutex_t mutex;
	pthread_cond_t new_job;
	jos_job *first;
	jos_job *free;
	jos_job_block *block;
	uint32_t num_queue;
	unsigned int num_threads;
	jos_thread thread[];
} jos_pool;

// Protoddtype(s)
static void* jos_thread_func(void*);

// Expands the number of available job items by addint new free items.
static void jos_new_job_block(jos_pool *pool, uint32_t jobs)
{
	// Just wait untill allocation succeeds.
	jos_job_block *block = NULL;
	while (!block)
	{
		block = (jos_job_block*) malloc(sizeof(jos_job_block) + sizeof(jos_job) * jobs);
		if (!block)
			sleep(1);
	}

	// Add this block to the free list
	block->next = pool->block;
	pool->block = block;

	// Add the job items to the free jobs list
	for (uint32_t i = 0; i < jobs - 1; i++)
		block->job[i].next = & block->job[i+1];
	block->job[jobs - 1].next = pool->free;
	pool->free = block->job;
}

// sched_new_pool returns a job scheduling threadpool of the specified number of threads or NULL if
// for some reason construction did not succeed. It will never return a smaller pool.
jos_pool* jos_new_pool(unsigned int num_threads)
{
	jos_pool *pool = malloc(sizeof(jos_pool) + sizeof(jos_thread) * num_threads);
	if (pool)
	{
		pool->num_threads = num_threads;
		pthread_mutex_init(& pool->mutex, NULL);
		pthread_condattr_t attr;
	//	pthread_condattr_init(&attr);
	//	pthread_condattr_setclock(&attr, JOS_CLOCK_SOURCE);
	//	pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
		pthread_cond_init(& pool->new_job, &attr);
		pool->first = NULL;
		pool->free = NULL;
		pool->block = NULL;
		jos_new_job_block(pool,JOS_BLOCK_SIZE);
		pthread_mutex_lock(&pool->mutex);

		// Create threads
		unsigned int i = 0;
		while (i < num_threads && pthread_create(& pool->thread[i].thread, NULL, &jos_thread_func, (void*) pool) == 0)
		{
			i++;
			pool->num_threads = i;
		}

		// Only run if all threads are created successfully
		pool->run = pool->num_threads == num_threads;

		// Set the threads free to start
		pthread_mutex_unlock(& pool->mutex);

		// Cleanup if not all threads are created successfully
		if (! pool->run)
		{
			for (i = 0; i < pool->num_threads; i++)
				pthread_join(pool->thread[i].thread, NULL);
			free(pool->block);
			free(pool);
			pool = NULL;
		}
	}
	return pool;
};

static void jos_stop_job(void *p) {(void)(p);};

// Deletes a pool.
void jos_delete_pool(jos_pool* pool)
{
	pthread_mutex_lock(& pool->mutex);
	pool->run = 0;
	pthread_mutex_unlock(& pool->mutex);
	for (unsigned int i = 0; i < pool->num_threads; i++)
		jos_run(pool, jos_stop_job, 0);	
	for (unsigned int i = 0; i < pool->num_threads; i++)
		pthread_join(pool->thread[i].thread, NULL);

	jos_job_block *b;
	while(pool->block)
	{
		b = pool->block;
		pool->block = b->next;
		free(b);
	}
	
	free(pool);
	pool = NULL;
};

// Compares two timespecs: less than or equal, "<=".
static int lte(const struct timespec *lhs, const struct timespec *rhs)
{
	if(lhs->tv_sec == rhs->tv_sec)
		return lhs->tv_nsec <= rhs->tv_nsec;
	else
		return lhs->tv_sec < rhs->tv_sec;
}

// Subtracts timespecs 1 = 2 - 3
void sub_ts(struct timespec *rv, const struct timespec *lhs, const struct timespec *rhs)
{
	rv->tv_sec = lhs->tv_sec - rhs->tv_sec;
	rv->tv_nsec = lhs->tv_nsec - rhs->tv_nsec;
	if (rv->tv_nsec < 0)
	{
		rv->tv_nsec += 1000000000;
		rv->tv_sec -= 1;
	}
}

// Internally each thread executes this function.
// There is a bit messy locks: The whole function assumes ownership of pool.
// Only, during waiting on a new job and during execution of the job the
// ownership is released. Intended side effect is that the thread is held during
// construction of the pool. If construction is partially successful (not
// all threads created) then run will guide the thread to exit immediately upon
// release of the mutex.
static void* jos_thread_func(void* p)
{
	jos_pool *pool = (jos_pool*) p;
	jos_job* job = NULL;
	struct timespec ts;
	int rv;

	pthread_mutex_lock(&pool->mutex);

	while(pool->run)
	{
		// Fetch a job
		if (! pool->first)
		{
			pthread_cond_wait(&pool->new_job, &pool->mutex);
			continue;
		}
		rv = clock_gettime(JOS_CLOCK_SOURCE, &ts);
		if (! lte( & pool->first->at, & ts))
		{
			sub_ts(&ts, & pool->first->at, &ts);
			//pthread_cond_time_asdf
		}
		job = pool->first;
		pool->first = job->next;

		// Perform the job
		pthread_mutex_unlock(&pool->mutex);
		job->fnc(job->arg);
		free(job);
		pthread_mutex_lock(&pool->mutex);
	}

	pthread_mutex_unlock(&pool->mutex);
	return p;
};

static jos_job* get_empty_job(jos_pool *pool)
{
	if (! pool->free)
		jos_new_job_block(pool, JOS_BLOCK_SIZE);
	jos_job *job = pool->free;
	pool->free = job->next;
	return job;
}

static void insert_job(jos_pool *pool, jos_job *job)
{
	// Goal is to insert the job based on the timespec, after all jobs of earlier or the same timespec,
	// and before any jobs with a later timespec.
	jos_job **i = & pool->first;
}


int jos_run(jos_pool* pool, void (*fnc)(void*), void* arg)
{
	if(pool)
	{
		jos_job* job = (jos_job*) malloc(sizeof(job));
		if(job)
		{
			job->fnc = fnc;
			job->arg = arg;
			job->next = NULL;

			pthread_mutex_lock(&pool->mutex);
			//if (pool->last)
			//	pool->last->next = job;
			//pool->last = job;
			if (! pool->first )
				pool->first = job;
			//pool->num_jobs++;
			pthread_mutex_unlock(&pool->mutex);
			
			pthread_cond_signal(&pool->new_job);
			return 0;
		}
		return 1;
	}
	return 2;
}


//void sched_at(void (*)(void*), void*, double);		// Schedules at timestamp.
//void sched_in(void (*)(void*), void*, float);		// Schedules after given time [s].
//void sched_every(void (*)(void*), void*, float);	// Repeats every given time [s]. Will be aligned to timestamp.
//void sched(int*);									// Scheduler's function. Returns when given run flag turns 0.

