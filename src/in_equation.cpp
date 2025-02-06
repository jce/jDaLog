#include "in_equation.h"

#include "floatLog.h"
#include "in.h"
#include "main.h"
#include <map>
#include "stdio.h"
#include <string>
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"

using namespace std;

#define DBG(...) printf(__VA_ARGS__);
//#define DBG(...)

// In version that derives its value from an equation. Equations use only constants and ins.

in_equation::in_equation(string d, equation *_eq, string n, string u, const unsigned int de) : in(d, n, u, de), eq(_eq) 
{
	eq->register_callback_on_update( cc_in_equation_call_on_equation_result_update, this );
	eq->register_callback_on_change( cc_in_equation_call_on_equation_result_change, this );
	eq->register_callback_on_turn_invalid( cc_in_turn_invalid, this );
	eq->register_callback_on_turn_valid( cc_in_equation_call_on_turn_valid, this );
}	

in_equation::~in_equation()
{
	delete eq;
}

void in_equation::getDataSummary(vector<flStat> &stats, unsigned int len, double from, double to)
{
	eq->get_summary(&stats, len, from, to);
}

void in_equation::call_on_equation_result_update()
{
}

void in_equation::call_on_equation_result_change()
{
}

void in_equation::call_on_turn_valid()
{
}


void build_in_equations(json_t *input_data)
{
	if (! json_is_object(input_data))
		return;
	json_t *json, *jdecimals;
	const char *id;
	const char *name;
	const char *unit;
	int decimals;
	equation *eq;

	json_object_foreach(input_data, id, json)
	{
		name = NULL;
		unit = NULL;
		decimals = 0;
		eq = NULL;

		if (json_is_object(json))
		{
			name = 	json_string_value(json_object_get(json, "name"));
			if (!name)
				name = id;
			unit =	json_string_value(json_object_get(json, "unit"));
			if (!unit)
				unit = "";
			jdecimals = json_object_get(json, "decimals");
			if (json_is_number(jdecimals))
				decimals = json_number_value(jdecimals);
			else
				decimals = 0;
			eq = equation_from_json(json);
			
			if (id and eq)
				new in_equation(id, eq, name, unit, decimals);
			else
				printf("Could not build in_equation for %s\n", id);
		}
	}
}

