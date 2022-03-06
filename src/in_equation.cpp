#include "in_equation.h"

#include "floatLog.h"
#include "in.h"
#include "main.h"
#include <map>
#include "mytime.h"	// now()
#include "stdio.h"
#include <string>
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"

using namespace std;

//#define DBG(...) printf(__VA_ARGS__);
#define DBG(...)

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

void in_equation::getDataSummary(vector<flStat>&, unsigned int, double, double)
{
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


