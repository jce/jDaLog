#ifndef HAVE_MAIN_H
#define HAVE_MAIN_H

#include "job_sched.h"

double now();				// Current time as unix timestamp.

#define dataDir			"./data/"

// Job scheduler
#define DEFAULT_SCHEDULER_NUM_THREADS 4
extern jos_pool *pool;

// Global config
extern bool prune_input;

#define STR(_X_) #_X_

#ifdef HAVE_S7											
#define S7INTERFACE INTERFACE(S7, interface_S7_from_json)
#else // HAVE_S7
#define S7INTERFACE
#endif // HAVE_S7

#ifdef HAVE_RPI
#define RPI_INTERFACE INTERFACE(pi_gpio, interface_rpi_gpio_from_json)
#else // HAVE_RPI
#define RPI_INTERFACE
#endif // HAVE_RPI

#define INTERFACES \
	/*			type,		factory function		  */\
	INTERFACE(	mb,			interface_mb_from_json)		\
	INTERFACE(	circulac,	interface_circulac_from_json)\
	INTERFACE(	solarlog,	interface_solarlog_from_json)\
	INTERFACE(	hs110,	    interface_hs110_from_json)\
	INTERFACE(	fijnstof,   interface_fijnstof_from_json)\
	INTERFACE(	dcmr_sensor,interface_dcmr_sensor_from_json)\
	INTERFACE(	dcmr,		interface_dcmr_from_json)\
	INTERFACE(	macp,		interface_macp_from_json)\
	INTERFACE(	host,		interface_host_from_json)\
	/* Partial lists based on precompiler options */	\
	S7INTERFACE											\
	RPI_INTERFACE										\
	// End INTERFACES

#endif // HAVE_MAIN_H
