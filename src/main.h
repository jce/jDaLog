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

extern bool globalControl;	// If this bool is set, all outputs are controlled by this program. Otherwise it is listening only.

#endif // HAVE_MAIN_H
