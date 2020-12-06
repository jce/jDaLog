// JOb Scheduler (JOS for short):
// - Manages a job pool of fixed size
// - Can be given jobs to be executed now or in the future
// - Preserves job order
// JCE, 29-11-2020

// Inspiration has been obtained from https://github.com/Pithikos/C-Thread-Pool
// JCE, 22-10-2019

#include "job_sched.h"

#include <math.h>		// modff
#include <pthread.h>	// mutex, condition variable
#include <stdio.h>
#include <stdlib.h>		// malloc()
#include <time.h>		// clock source macro's
#include <unistd.h>		// sleep

// Individually scheduled job item.
typedef struct jos_job
{
	struct jos_job *next;
	struct timespec at;
	double interval;
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

// Prototype(s)
static void* jos_thread_func(void*);
void next_multiple_of(struct timespec*, double);
static void insert_job(jos_pool*, jos_job*);

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

// jos_new_pool returns a job scheduling threadpool of the specified number of threads or NULL if
// for some reason construction did not succeed. It will never return a smaller pool.
jos_pool* jos_new_pool(unsigned int num_threads)
{
	jos_pool *pool = (jos_pool*) malloc(sizeof(jos_pool) + sizeof(jos_thread) * num_threads);
	if (pool)
	{
		pool->num_threads = num_threads;
		pthread_mutex_init(& pool->mutex, NULL);
		pthread_condattr_t attr;
		pthread_condattr_init(&attr);
		pthread_condattr_setclock(&attr, JOS_CLOCK_SOURCE);
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

static void jos_stop_job(void *p) 
{
	(void)(p);
	printf("running stop job\n");
};

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
};

// Compares two timespecs: less than, "<".
static int lt_ts(const struct timespec *lhs, const struct timespec *rhs)
{
	if (lhs->tv_sec < rhs->tv_sec)
		return 1;
	if (lhs->tv_sec == rhs->tv_sec)
		return lhs->tv_nsec < rhs->tv_nsec;
	return 0;
}

// Internally each thread executes this function.
// The whole function assumes ownership of pool.
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

	pthread_mutex_lock(&pool->mutex);

	while(pool->run)
	{
		// Fetch a job
		if (! pool->first)
		{
			pthread_cond_wait(&pool->new_job, &pool->mutex);
			continue;
		}
		job = pool->first;
		clock_gettime(JOS_CLOCK_SOURCE, &ts);

		if ( lt_ts( &ts, & job->at))
		{
			pthread_cond_timedwait(&pool->new_job, &pool->mutex, &job->at);
			continue;
		}
		pool->first = job->next;

		// To be able to process the next scheduled item while this job is not yet finished,
		// push one other thread in the timedwait.
		if (pool->first)
			pthread_cond_signal(&pool->new_job);
		
		// Perform the job
		pthread_mutex_unlock(&pool->mutex);
		job->fnc(job->arg);
		pthread_mutex_lock(&pool->mutex);

		// If the job is a regular job, reschedule it.
		if (job->interval)
		{
			// Take the current time (so after running of the job) to prevent multiple jobs running
			// at the same time, or programming errors to fill up the whole thread pool.
			// This will result in skipping (an) interval(s), if execution of the previous interval took too long.
			clock_gettime(JOS_CLOCK_SOURCE, &ts);
			next_multiple_of(&ts, job->interval);
			job->at = ts;
			insert_job(pool, job);
		}
		else
		{		
			// The job is a one-shot job. Return the job struct to the free list.
			job->next = pool->free;
			pool->free = job;
		}
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
	// Insert the job based on the timespec, after all jobs of earlier or the same timespec,
	// and before any jobs with a later timespec.
	jos_job **i = & pool->first;
	while ( (*i) && ! lt_ts( &job->at, &(*i)->at) )
		i = & (*i) -> next;
	job->next = (*i);
	(*i) = job;
}

void jos_run_at(jos_pool* pool, const struct timespec *ts, void (*fnc)(void*), void* arg)
{
	pthread_mutex_lock(&pool->mutex);
	jos_job* job = get_empty_job(pool);
	job->at = *ts;
	job->interval = 0;
	job->fnc = fnc;
	job->arg = arg;
	insert_job(pool, job);
	pthread_mutex_unlock(&pool->mutex);
		
	pthread_cond_signal(&pool->new_job);
}

void jos_run(jos_pool *pool, void (*fnc)(void*), void* arg)
{
	struct timespec ts;
	clock_gettime(JOS_CLOCK_SOURCE, &ts);
	jos_run_at(pool, &ts, fnc, arg);
}

void jos_run_in(jos_pool *pool, float t, void (*fnc)(void*), void* arg)
{
	struct timespec ts;
	clock_gettime(JOS_CLOCK_SOURCE, &ts);
	ts.tv_sec += t;
	ts.tv_nsec += fmodf(t, 1) * 1000000000;
	if (ts.tv_nsec >= 1000000000)
	{
		ts.tv_sec += 1;
		ts.tv_nsec -= 1000000000;
	}
	jos_run_at(pool, &ts, fnc, arg);	
}

// Helper function, finds the next timespec that is multiple of interval.
void next_multiple_of(struct timespec *ts, double interval)
{
	double time = (double) ts->tv_sec + (double) ts->tv_nsec / 1000000000;
	modf(time / interval, &time);
	time = (time + 1) * interval;
	ts->tv_sec = time;
	ts->tv_nsec = fmod(time, 1) * 1000000000; 
}

void jos_run_every(jos_pool* pool, double interval, void (*fnc)(void*), void* arg)
{
	struct timespec ts;
	clock_gettime(JOS_CLOCK_SOURCE, &ts);
	next_multiple_of(&ts, interval);

	pthread_mutex_lock(&pool->mutex);
	jos_job* job = get_empty_job(pool);
	job->at = ts;
	job->interval = interval;
	job->fnc = fnc;
	job->arg = arg;
	insert_job(pool, job);
	pthread_mutex_unlock(&pool->mutex);
		
	pthread_cond_signal(&pool->new_job);
}

// debug print function
void jos_print(jos_pool *pool)
{
	pthread_mutex_lock(&pool->mutex);
	double time;
	jos_job *job = pool->first;
	printf("     fnc      arg               at         interval\n");
	while(job)
	{
		time = (double) job->at.tv_sec + (double) job->at.tv_nsec / 1000000000;
		printf("%p %p %16f %16f\n", job->fnc, job->arg, time, job->interval);
		job = job->next;
	}
	pthread_mutex_unlock(&pool->mutex);
}
