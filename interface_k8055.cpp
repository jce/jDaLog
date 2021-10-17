#include "endian.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_k8055.h"
#include "k8055.h"
#include "main.h"
#include "tcmath.h"
#include "timefunc.h"
#include "math.h" // pow()
#include "out.h"
#include "stdio.h"
#include <string>
#include "string.h"
#include "stringStore.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include <unistd.h>		// sleep

#define DBG(...)
//#define DBG(...) { printf(__VA_ARGS__); printf("\n"); }

using namespace std;

interface_k8055::interface_k8055(const string d, const string n):interface(d, n, 1)
{
	ai1 = new in("k8055_ai1", "k8055 ai1", "", 3);
	ai1->set_valid_time(2.5 * 60.0);
	bf = new_blockfilter(40);
	ao1 = new out("k8055_ao1", "k8055 ao1", "", 3, this, 0, 1, 256);
	ao2 = new out("k8055_ao2", "k8055 ao2", "", 3, this, 0, 1, 256);
}

interface_k8055::~interface_k8055()
{
	delete ai1;
	delete ao1;
	delete ao2;
	free(bf);
}

void interface_k8055::setOut(out *o, float v)
{
	if (v < 0)
		v = 0;
	if (v > 1)
		v = 1;
	if (o == ao1)
		outval1_new = v;
	if (o == ao2)
		outval2_new = v;
}

void interface_k8055::run()
{
	const float hysteresis = 0.0014; 				// Value hysteresis
	float hysteresis_val;							// Average value during hysteresis
	double hysteresis_t;							// Time at writing the hysteresis value
	//float last_val;									// Last measurement value
	//double last_t;									// Time at measuring last value
	const float write_hysteresis_start = 0.1; 		// [s] If hysteresis is exceeded and the value is older than this time: write
	const float max_write_time = 60; 				// [s] No gaps of over this time are allowed, writes a new value within this time.	

	int result, prevresult;
	long a1l, a2l;
	result = OpenDevice(0);
	if (result < 0)
	{
		DBG("Could not open the k8055 (port:%d). Please ensure that the device is correctly connected.",0);
	}
	while(result < 0)
	{
		if (!run_flg)
			return;
		result = OpenDevice(0);
		usleep(1000000);
	}
	while(run_flg)
	{
		prevresult = result;

		if (result >= 0)
		{
			result = ReadAllAnalog(&a1l, &a2l);
		}

		if ( (result >= 0) && (outval1_new != outval1_written) )
		{ 
			result = OutputAnalogChannel(1, outval1_new * 255);
			if (result >= 0)
			{
				outval1_written = outval1_new;
				ao1->setValue(outval1_written);
			}
		}

		if ( (result >= 0) && (outval2_new != outval2_written) )
		{ 
			result = OutputAnalogChannel(2, outval2_new * 255);
			if (result >= 0)
			{
				outval2_written = outval2_new;
				ao2->setValue(outval2_written);
			}
		}

		// Try and recover from a disconnection
		if (result < 0 && prevresult >= 0)
		{
			blockfilter_clear(bf);
			DBG("k8055: Connection failed");
			outval1_written = 0;
			outval2_written = 0;
		}
		if (result >= 0)
		{
			//DBG("Read value: %ld %ld", a1l, a2l);
			blockfilter_new_val(bf, a1l);
			if (blockfilter_filled(bf))
			{
				float val = blockfilter_get_average(bf) / 255;
				//DBG("Filtered value: %f", val);
				double t = now(); //get_time_monotonic(); Should be monotonic but this complicates things when writing the value for a past point in time.
				if (! hysteresis_t)
				{
					hysteresis_val = val;
					hysteresis_t = t;
					//last_val = val;
					//last_t = t;
					DBG("Write first value: %f", hysteresis_val);
					ai1->setValue(hysteresis_val, hysteresis_t);
				}
				if (t > hysteresis_t + max_write_time)
				{
					// Write the last previous measurement
					//hysteresis_val = last_val;
					//hysteresis_t = last_t;
					hysteresis_t = t;
					DBG("Write keepalive value: %f", hysteresis_val);
					ai1->setValue(hysteresis_val, hysteresis_t);
				}
				else if ((t > hysteresis_t + write_hysteresis_start) && (val > (hysteresis_val + hysteresis) || val < (hysteresis_val - hysteresis)))
				{
					hysteresis_val = val;
					hysteresis_t = t;
					DBG("Write changed value: %f", hysteresis_val);
					ai1->setValue(hysteresis_val, hysteresis_t);
				}
				//last_val = val;
				//last_t = t;
			}
		}
		if (result < 0)
		{
			CloseDevice(); 
			result = OpenDevice(0);	// Added a hack to the library, where OpenDevice() also re-scans the bus.
			if (result >= 0)
			{
				DBG("k8055: Connection restored");
			}
		}
		//usleep(1000);
		//t = now_mt();
	}
	CloseDevice();
} 

// Factory for interface_k8055.
void interface_k8055_from_json(const char *ifid, const char *ifname, json_t*)
{
	// Lets make only one. This inplementation is not realy meant to be flexible.
	static interface *k8055 = NULL;
	if (!k8055)
		k8055 = new interface_k8055(ifid, ifname);
}


