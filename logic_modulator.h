#ifndef HAVE_MODULATOR_H
#define HAVE_MODULATOR_H

#include "stdio.h"
#include <string>
#include "stringStore.h"
#include "out.h"
#include "outhost.h"
#include "pthread.h"
#include "logic.h"
#include "main.h"
#include "jansson.h"

typedef struct bool_in_double
{
	bool have = false;
	in *i = NULL;
	double d = 0;
} in_double;

class logic_modulator: public logic {
	public:
		logic_modulator(const std::string, const std::string, out*); // id, name, out to be modulated.
		virtual ~logic_modulator();
		virtual void setOut(out*, float);
		int make_page(struct mg_connection*);
		//bool_in_double error_sum_limit_low, error_sum_limit_high, time_off_min, time_off_max, time_on_min, time_on_max;
		bool_in_double time_off_min, time_on_min;
	private:
		double time_to_next(double, double, double); // Calculates time to next. 1 - setpoint, 2 - output, 3 - error [s]. returns time [s]
		void run();
		CC(logic_modulator, run);
		out *out_mod, *out_sp = NULL;
		double e = 0;
		double t = 0;
		double out_val = 0, sp_val = 0;
		double last_transition = 0;
		pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	};

// Helper function for building object
void bool_in_double_from_json(bool_in_double&, json_t*);
bool bid_value(bool_in_double&); // Fills bid.value if the bit is an "in", returns bid.have.
void logic_modulator_from_json(const char*, const char*, json_t*);

#endif // HAVE_MODULATOR_H
