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
}	

in_equation::~in_equation()
{
	delete eq;
}

