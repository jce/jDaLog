#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "logic_modulator.h"
#include "main.h"
#include "sys/stat.h" 	// mkdir
#include "timefunc.h"
#include "out.h"
#include "unistd.h" 	// usleep
#include "math.h"		// pow
#include "webin.h"
#include "webgui.h"		// plotlines
#include "float.h"		// FLT_MIN

using namespace std;

void test(void *v)
{
	printf("test %p\n", v);
}

logic_modulator::logic_modulator(const string d, const string n, out *o) : logic(d, n), out_mod(o) 
{
	out_sp = new out(d + "_sp", n + " setpoint", out_mod->getUnits(), 4, (void*) this, 0, FLT_MIN, 1);
	//out_sp->register_callback_on_update( (void(*)(void*)) cc_run, (void*) this);
	t = get_time_monotonic();

/*
	out_sp->register_callback_on_update( test, (void*) 1 );
	out_sp->register_callback_on_change( test, (void*) 2);
	out_sp->register_callback_on_turn_invalid( test, (void*) 3);
	out_sp->register_callback_on_turn_valid( test, (void*) 4 );
*/
}


logic_modulator::~logic_modulator()
{
	if(out_sp)
		delete out_sp;
}

double logic_modulator::time_to_next(double sp, double out, double e)
{
	if (sp == out)
		return 60.0;
	double rv = e / (sp - out);
	if (rv < 0.0)
		rv = 0.0;
	if (sp > out) // Approaching an on transition
		if (bid_value(time_off_min))
			if (rv < time_off_min.d)
				rv = time_off_min.d;
	if (sp < out) // Approaching an off transition
		if (bid_value(time_on_min))
			if (rv < time_on_min.d)
				rv = time_on_min.d;
	// Keep a regular call.
	//if (rv > 1.0)
	//	rv = 1.0;
	return rv;
}

void logic_modulator::run()
{
	// Time and time delta.
	double tnew = get_time_monotonic();
	double dt = tnew - t;
	t = tnew;

	// Setpoint.
	sp_val = sp_val_new;
	sp_val_new = out_sp->getValue();

	// Error integration.
	e += dt * (out_val - sp_val);

	// Comparator.
	out_val_new = e <= 0;
	printf("%f %f %f \n", sp_val_new, out_val_new, e); 
	
	// Next time
	double tnext = t + time_to_next(sp_val_new, out_val_new, e);
	jos_remove(pool, cc_run, this);
	jos_run_at(pool, tnext, cc_run, this);
		
	// No switch.
	if (out_val == out_val_new)
		return;

	// Switch.
	out_mod->setOut(out_val_new);
	out_val = out_val_new;
	last_transition = t;
}

void logic_modulator::setOut(out *o, float f)
{
	if (o == out_sp)	
		out_sp->setValue(f);
	jos_remove(pool, cc_run, this);
	jos_run(pool, cc_run, this);
}

int logic_modulator::make_page(struct mg_connection *conn)
{
	string s;
	s += make_out_link(out_sp);
	mg_printf(conn, s.c_str());
/*
	mg_printf(conn, "on/off regulator page. This page displays the current state as well as configured values for this on-off regulator<br>\n");
	mg_printf(conn, "Lower treshold: %.*f %s<br>\n", _low->getDecimals(), _low->getValue(), _low->getUnits().c_str());
	mg_printf(conn, "Higher treshold: %.*f %s<br>\n", _high->getDecimals(), _high->getValue(), _high->getUnits().c_str());
	mg_printf(conn, "Error value: %.*f %s<br>\n", _e->getDecimals(), _e->getValue(), _e->getUnits().c_str());
	mg_printf(conn, "Output value: %.*f %s<br>\n", _out->getDecimals(), _out->getValue(), _out->getUnits().c_str());

	string line;
 	line = make_image_line(plotLines(_low, _high, _e, _out, now() - 3600, now(), 1280, 300, "low, high, error and output"));
	mg_printf(conn, line.c_str());
 	//line = make_image_line(plotLines(_low, _high, _e, _out, now() - 24*3600, now(), 1280, 300, "low, high, error and output"));
	//mg_printf(conn, line.c_str());
	return 1;
	*/	
	return 1;
}

// helper functions

void bool_in_double_from_json(bool_in_double &bid, json_t *json)
{
	if(json_is_number(json))
	{
		bid.have = true;
		bid.d = json_number_value(json);
		return;
	}
	const char *s = json_string_value(json);
	in *i = get_in(s);
	if (i)
	{
		bid.have = true;
		bid.i = i;
	}
}

bool bid_value(bool_in_double &bid)
{
	if (bid.have)
		if (bid.i)
			bid.d = bid.i->getValue();
	return bid.have;
}

void logic_modulator_from_json(const char *id, const char *name, json_t *json)
{
	if (!id)
	{
		printf("logic_modulator: no id given.\n");
		return;
	}	
	if (!name)
		name = id;
	const char *out_mod_s = json_string_value(json_object_get(json, "out"));
	out *mod_out = get_out(out_mod_s);
	if (!mod_out)
	{
		printf("logic_modulator %s: no (valid) out specified: %s\n", id, out_mod_s);
		return;
	}
	logic_modulator *m = new logic_modulator(id, name, mod_out);
	//bool_in_double_from_json(m->error_sum_limit_low,	json_object_get(json, "error_sum_limit_low"));
	//bool_in_double_from_json(m->error_sum_limit_high, 	json_object_get(json, "error_sum_limit_high"));
	bool_in_double_from_json(m->time_off_min, 			json_object_get(json, "time_off_min"));
	//bool_in_double_from_json(m->time_off_max, 			json_object_get(json, "time_off_max"));
	bool_in_double_from_json(m->time_on_min, 			json_object_get(json, "time_on_min"));
	//bool_in_double_from_json(m->time_on_max, 			json_object_get(json, "time_on_max"));
}

