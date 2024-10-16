#ifndef HAVE_MAIN_H
#define HAVE_MAIN_H

#include "job_sched.h"

#define tcProgramName		"tcFarmControl"
#define tcProgramVersion	0.9	// version 0.8 and higher are C++, pre are Python 2.7. 0.9: binary in files are now stored ordered.
#define tcDataDir			"./data/"
#define tcWebRoot			"./http/"

// Webserver
#define NUM_THREADS 		2

// Job scheduler
#define DEFAULT_SCHEDULER_NUM_THREADS 4
extern jos_pool *pool;

// Global config
extern bool prune_input;

#define STR(_X_) #_X_

#ifdef HAVE_S7											
#define S7INTERFACE \
	INTERFACE(	S7,			interface_S7_from_json		)
#else // HAVE_S7
#define S7INTERFACE
#endif // HAVE_S7

#define INTERFACES \
	/*			type,		factory function			*/\
														\
	/* Partial lists based on precompiler options */	\
	S7INTERFACE											\
	// End INTERFACES

#endif // HAVE_MAIN_H
