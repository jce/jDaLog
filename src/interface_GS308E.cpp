#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_GS308E.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "modbus/modbus.h"
#include "out.h"
#include "timefunc.h"

// Quite a hackish approach to reading the bandwidth on an NetGear GS308E smart switch. The hack is that a separate python script is
// running to fetch the actual data. Difficulties with logging in, hashed passwords, sessions and cookies. Apparently the smart
// part was just made to be used by humans, and not to be used in some automation...
// Anyway the script has to be running (started separately). It generates a status file in the temp dir, overwriting each time
// with new data. The script tries to align writes on the second boundary of the monotonic clock. We aim to read at 0.5 sec later.


//#define DBG(...) printf(__VA_ARGS__);
#define DBG(...)

using namespace std;

interface_gs308e::interface_gs308e(const string d, const string n, const string ipstr):interface(d, n, 1), \
	filestr("/tmp/status_" + ipstr + ".txt")
{
	char name[100], descr[100];
	for (uint16_t i = 0; i<8; i++)
	{
		snprintf(descr, 100, "_%urx", i+1);
		snprintf(name, 100, " %urx", i+1);
		inp[i] = new in(getDescriptor() + descr, getName() + name, "bps", 0);
		snprintf(descr, 100, "_%utx", i+1);
		snprintf(name, 100, " %utx", i+1);
		inp[i+8] = new in(getDescriptor() + descr, getName() + name, "bps", 0);
		snprintf(descr, 100, "_%uer", i+1);
		snprintf(name, 100, " %uer", i+1);
		inp[i+16] = new in(getDescriptor() + descr, getName() + name, "bps", 0);
		inp[i+16]->hidden = true;
	}
}

interface_gs308e::~interface_gs308e()
{
	for (int i = 0; i<24; i++)
		delete inp[i];
}

void interface_gs308e::getIns()
{
	DBG("hi\n");
	double t1, t2;
	double d[3*8];	// rx 1-8, tx 1-8, er 1-8 readout.
	int cnt;
	double clock_offset = now() - get_time_monotonic();
	
	FILE *f = fopen(filestr.c_str(), "r");
	DBG("opening %s: %p\n", filestr.c_str(), f);
	if (f)
	{
		cnt = fscanf(f, "%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf\n%lf", \
				&t1, \
				&d[0], &d[8],  &d[16], \
				&d[1], &d[9],  &d[17], \
				&d[2], &d[10], &d[18], \
				&d[3], &d[11], &d[19], \
				&d[4], &d[12], &d[20], \
				&d[5], &d[13], &d[21], \
				&d[6], &d[14], &d[22], \
				&d[7], &d[15], &d[23], \
				&t2 );
		fclose(f);
		DBG("read %d items from file. t1 = %f, t2 = %f, t_prev = %f\n", cnt, t1, t2, t_prev);
		if (cnt == 26 && t1 == t2 && t1 != t_prev && t1 != 0)
		{
			DBG("passed sanity check\n");
			double dt = t1 - t_prev;
			t_prev = t1;
			if (dt < 10)	// Do not count intervals larger than 10 seconds
			for (int i = 0; i < 24; i++)
				if (d[i] >= d_prev[i])	// Skip backcounts
					inp[i]->setValue( (d[i] - d_prev[i]) * 8 / dt , t1 + clock_offset );	// Bits per second.
			memcpy(d_prev, d, 18 * sizeof(double));
		}
	}
}


void interface_gs308e_from_json(const char *ifid, const char *ifname, json_t *json)
{
	if (!ifid)
	{
		printf("interface_gs308e: no id given.\n");
		return;
	}	
	if (!ifname)
		ifname = ifid;
	const char *ipstr = json_string_value(json_object_get(json, "ip"));
	if (!ipstr)
	{
		printf("interface_gs308e: no ip given.\n");
		return;
	}
	new interface_gs308e(ifid, ifname, ipstr);
}
