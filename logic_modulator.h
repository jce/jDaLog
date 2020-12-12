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
		void run();
		CC(logic_modulator, run);
		virtual void setOut(out*, float);
		int make_page(struct mg_connection*);
		bool_in_double error_sum_limit_low, error_sum_limit_high, time_off_min, time_off_max, time_on_min, time_on_max;
	private:
		out *out_mod, *out_sp = NULL;
	};

// Helper function for building object
void bool_in_double_from_json(bool_in_double&, json_t*);
void logic_modulator_from_json(const char*, const char*, json_t*);

#endif // HAVE_MODULATOR_H
