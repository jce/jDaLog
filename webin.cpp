#include "stdio.h"
#include <string>
#include "floatLog.h"
#include "webin.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "mytime.h"	// now()
#include "stringStore.h"
#include <map>

using namespace std;

#define debug

// Implementation of class in. In should give an uniform representation of inputs for tcFarmControl. I assume to use it later for both visualisation and as generic origin of information for internal logics. The in should also allow for "simple" enumeration and finding of any of the inputs. To be done by an in-map or list with search function. JCE, 19-6-13

map<string, webin*> webinmap;

webin::webin(const string d, const string n, const string u, const unsigned int de) : in(d, n, u, de)
{
	setValid(true);	// there is no such thing as an invalid web-in. It is like a configuration value.
	webinmap[getDescriptor()] = this;
}

webin::~webin()
{
}

void webin::setValue(float v, double t)
{
	if (t == 0) 
		t = now();
	in::setValue(_value, t-0.001);
	in::setValue(v, t);
}

void touchAllWebins()
{
	map<string, webin*>::iterator i;
	for (i = webinmap.begin(); i!= webinmap.end(); i++)
		i->second->touch();
}
