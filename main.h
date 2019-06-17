#ifndef HAVE_MAIN_H
#define HAVE_MAIN_H

#define tcProgramName		"tcFarmControl"
#define tcProgramVersion	0.800	// version 0.8 and higher are C++, pre are Python 2.7
#define tcDataDir		"./data/"
#define tcWebRoot		"./http/"

// Webserver
#define NUM_THREADS 		2

extern bool globalControl;	// If this bool is set, all outputs are controlled by this program. Otherwise it is listening only.

#endif // HAVE_MAIN_H
