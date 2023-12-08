#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_circulac.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "out.h"

//#define DBG(...)
#define DBG(...) { printf(__VA_ARGS__); printf("\n"); }

using namespace std;

interface_circulac::interface_circulac(const string d, const string n, float i, const string _devstr):interface(d, n, i), devstr(_devstr)
{
	DBG("Created interface_circulac with name %s and device %s", getName().c_str(), devstr.c_str());
}

interface_circulac::~interface_circulac()
{
}

void interface_circulac::getIns()
{
	//double start = now();
}


bool interface_circulac_from_json(const json_t *json)
{
	#define JSTR(_X_)	json_string_value(json_object_get(json, #_X_))
	const char *id = JSTR(id);
	const char *name = JSTR(name);
	if (not name)
		name = id;
	const char *devstr = JSTR(device);
	#undef JSTR

	if (id and devstr)
	{
		new interface_circulac(id, name, 1, devstr);
		return true;
	}
	return false;
	
}
