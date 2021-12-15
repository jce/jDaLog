#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_fijnstof.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "out.h"

#define debug

using namespace std;

interface_fijnstof::interface_fijnstof(const string d, const string n, float interval, const string ipstr):interface(d, n, interval), _ipstr(ipstr){
	//in *requests, *resets, *scanrate, *uptime, *latency;
	pm2 = new in(getDescriptor() + "_pm2.5", getName() + " pm2.5", "ug/m3", 1);
	pm10 = new in(getDescriptor() + "_pm10", getName() + " pm10", "ug/m3", 1);
	temp = new in(getDescriptor() + "_t", getName() + " temperature", "degC", 1);
	rh = new in(getDescriptor() + "_rh", getName() + " humidity", "%", 1);
	pressure = new in(getDescriptor() + "_p", getName() + " pressure", "hPa", 2);
	wifi_str = new in(getDescriptor() + "_ws", getName() + " wifi strength", "dBm");
	wifi_qua = new in(getDescriptor() + "_wq", getName() + " wifi quality", "%");
	samples = new in(getDescriptor() + "_smp", getName() + " samples", "");
	time_sending = new in(getDescriptor() + "_ts", getName() + " time sending", "ms");
	fw_ver = new in(getDescriptor() + "_fw", getName() + " firmware version", "");
	errors = new in(getDescriptor() + "_er", getName() + " errors", "");
	latency = new in(getDescriptor() + "_lt", getName() + " latency", "ms", 3);

	pm2->set_valid_time(360);
	pm10->set_valid_time(360);
	}

interface_fijnstof::~interface_fijnstof(){
	delete latency;
	delete errors;
	delete fw_ver;
	delete time_sending;
	delete samples;
	delete wifi_qua;
	delete wifi_str;
	delete pressure;
	delete rh;
	delete temp;
	delete pm10;
	delete pm2;
	}

void interface_fijnstof::getIns()
{
	double start = now();
	string page = get_html_page(_ipstr + "/values");
	double end = now();
	double t = (start + end) / 2;
	float f;

	latency->setValue((end-start)*1000, t);
	
	if (findFloatAfter(page, "Tijdsduur opsturen metingen</td><td class='r'>", f))
		time_sending->setValue(f, t);
	
	if (findFloatAfter(page, "enkel foutmeldingen</td><td class='r'>", f))
		errors->setValue(f, t);

	if (findFloatAfter(page, "<td>WiFi</td><td>Signaalsterkte</td><td class='r'>", f))
		wifi_str->setValue(f, t);
				
	if (findFloatAfter(page, "<td>WiFi</td><td>Signaalkwaliteit</td><td class='r'>", f))
		wifi_qua->setValue(f, t);

	int time_since_last_measurement;
	if (findIntAfter(page, "Huidige data</h4><b>", time_since_last_measurement))
		if (t - time_since_last_measurement > time_at_last_measurement + 10.0 )
		{
			//nr_samples = new_nr_samples;
			//int time_since_last_measurement;
			//if (findIntAfter(page, "Huidige data</h4><b>", time_since_last_measurement))
			{
				t -= time_since_last_measurement;

				// firmware volgt op "<br/>Firmware-versie: " en eindigt bij "<"
				const char *start, *end;
				char cue[] = "<br/>Firmware-versie: ";
				start = strstr(page.c_str(), cue);
				if(start)
				{
					start += strlen(cue);
					end = strstr(start, "<");
					if (end and end > start)
					{
						//printf("Firmware: %.*s\n", end-start, start);
						// The current firmware is NRZ-2019-127-1, how to convert this back to an numeric value?
						// Lets take every integer and concatenate this: 20191271
						// Then ignore the 20: 191271 This will fit in an float
						int a, b, c, num;
						num = sscanf(start, "%*6c%2d%*c%3d%*c%d", &a, &b, &c); 			// ______20-129-1
						if(num == 2)
							num = sscanf(start, "%*6c%2d%*c%3d%*c%*c%d", &a, &b, &c); 	// ______20-129-a1
						if(num == 2)													// ______20-129
							fw_ver->setValue((float) 10.0*b + 10000.0*a);
						if(num == 3)
							fw_ver->setValue((float) c + 10.0*b + 10000.0*a);
					}
				}
 
				if (findFloatAfter(page, "<td>SDS011</td><td>PM2.5</td><td class='r'>", f))
					pm2->setValue(f, t);
				
				if (findFloatAfter(page, "<td>SDS011</td><td>PM10</td><td class='r'>", f))
					pm10->setValue(f, t);
				
				if (findFloatAfter(page, "<td>BMP/E280</td><td>Temperatuur</td><td class='r'>", f))
					temp->setValue(f, t);
				
				if (findFloatAfter(page, "<td>BMP/E280</td><td>Luchtdruk</td><td class='r'>", f))
					pressure->setValue(f, t);
				
				if (findFloatAfter(page, "<td>BMP/E280</td><td>Rel. luchtvochtigheid</td><td class='r'>", f))
					rh->setValue(f, t);
				
				samples->setValue(nr_samples, t);

				time_at_last_measurement = t;
			}	
	}
}
