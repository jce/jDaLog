#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_dcmr_sensor.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "out.h"
#include <unistd.h>

#define debug

using namespace std;

interface_dcmr_sensor::interface_dcmr_sensor(const string d, const string n, float interval, const string ipstr):interface(d, n, interval), _ipstr(ipstr)
{
	LAeq = new in(getDescriptor() + "_le", getName() + " LAeq", "dB(A)", 1);
	LAmin = new in(getDescriptor() + "_li", getName() + " LA min", "dB(A)", 1);
	LAmax = new in(getDescriptor() + "_la", getName() + " LA max", "dB(A)", 1);
	wifi_str = new in(getDescriptor() + "_ws", getName() + " wifi strength", "dBm");
	wifi_qua = new in(getDescriptor() + "_wq", getName() + " wifi quality", "%");
	latency = new in(getDescriptor() + "_lt", getName() + " latency", "ms", 3);
}

interface_dcmr_sensor::~interface_dcmr_sensor()
{
	delete latency;
	delete wifi_qua;
	delete wifi_str;
	delete LAmax;
	delete LAmin;
	delete LAeq;
}

void interface_dcmr_sensor::run()
{
	float f;
	double next = now();
	while ( run_flg ) 
	{
		double start = now();
		if (next > start)
		{
			sleep(0.2);
			continue;
		}

		string page = get_html_page(_ipstr + "/values", 5);
		double end = now();
		double t = (start + end) / 2;

		latency->setValue((end-start)*1000, t);
	
		double since_last_measurement = 4;
		findFloatAfter(page, "Current data</h4><b>", f);
		since_last_measurement = f;
		
		// This data is min, average, max for a timesection of 10 seconds that ended
		// at since_last_measurement. Lets use the data production time as measurement
		// time, instead of the average sample time.
		t -= since_last_measurement; // + 5;
		
		// The next measurement should be at about 40% of the measurement interval,
		// 4 seconds. 
		next = end + 10 - since_last_measurement + 4;
	
		if (findFloatAfter(page, "LAeq</td><td class='r'>", f))
			LAeq->setValue(f, t);

		if (findFloatAfter(page, "LA min</td><td class='r'>", f))
			LAmin->setValue(f, t);

		if (findFloatAfter(page, "LA max</td><td class='r'>", f))
			LAmax->setValue(f, t);
		
		if (findFloatAfter(page, "signal strength</td><td class='r'>", f))
			wifi_str->setValue(f, t);
					
		if (findFloatAfter(page, "signal quality</td><td class='r'>", f))
			wifi_qua->setValue(f, t);
	}
}

interface_dcmr_sensor* interface_dcmr_sensor_from_json(const json_t *json )
{
	#define JSTR(_X_)	json_string_value(json_object_get(json, #_X_))
	#define JNR(_X_)	json_number_value(json_object_get(json, #_X_))
	#define JIN(_X_)	json_is_number(json_object_get(json, #_X_))
		
	const char *id = JSTR(id);
	const char *name = JSTR(name);
	if (not name)
		name = id;
	float scan = JNR(scan);
	const char *addr = JSTR(address);
	
	if (id and name and JIN(scan) and addr)
		return new interface_dcmr_sensor(id, name, scan, addr);
	printf("could not build interface_dcmr_sensor(%s, %s, %f, %s)\n", id, name, scan, addr);
	return NULL;	
}
