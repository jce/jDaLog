#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_rwl.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"

using namespace std;


interface_rwl::interface_rwl(const string d, const string n, float i, const string ipstr):interface(d, n, i), _ipstr(ipstr){
	waterlevel = new in(getDescriptor() + "_wl", getName() + " water level", "m", 3);
	watervolume = new in(getDescriptor() + "_wv", getName() + " water volume", "m3", 3);
	requestcounter = new in(getDescriptor() + "_rq", getName() + " request counter", "");
	resetcounter = new in(getDescriptor() + "_rs", getName() + " reset counter", "");
	scanrate = new in(getDescriptor() + "_sr", getName() + " scanrate", "Hz");
	uptime = new in(getDescriptor() + "_ut", getName() + " uptime", "s");
	latency = new in(getDescriptor() + "_lt", getName() + " latency", "ms", 3);
	adcreadout = new in(getDescriptor() + "_ar", getName() + " adc readout", "");
}

interface_rwl::~interface_rwl(){
	delete(waterlevel);
	delete(watervolume);
	delete(requestcounter);
	delete(resetcounter);
	delete(scanrate);
	delete(uptime);
	delete(latency);
	delete(adcreadout);}

void interface_rwl::getIns(){
	double start = now();
	string url = _ipstr + "/status.htm";
	string statusPage = get_html_page(url);
	string debugPage = get_html_page(_ipstr + "/debug.htm");
	double end = now();
	double t = (start + end) / 2;
	latency->setValue((end-start)/2*1000, t); // 2 requests, scale = ms
	float f;
	if (findFloatAfter(statusPage, "Water level (h) = ", f)) 
		waterlevel->setValue(f, t);
	else
		waterlevel->setValid(false);

	if (findFloatAfter(statusPage, "Water volume (v) = ", f)) 
		watervolume->setValue(f, t);
	else
		watervolume->setValid(false);

	if (findFloatAfter(statusPage, "drmod: ", f)) 
		adcreadout->setValue(f, t);
	else
		adcreadout->setValid(false);

	if (findFloatAfter(debugPage, "Requests: ", f)) 
		requestcounter->setValue(f, t);
	else
		requestcounter->setValid(false);

	if (findFloatAfter(debugPage, "Resets: ", f)) 
		resetcounter->setValue(f, t);
	else
		resetcounter->setValid(false);

	if (findFloatAfter(debugPage, "Scans per second: ", f)) 
		scanrate->setValue(f, t);
	else
		scanrate->setValid(false);

	if (findFloatAfter(debugPage, "Uptime [s, d h:m:s]: ", f)) 
		uptime->setValue(f, t);
	else
		uptime->setValid(false);


	//printf("Interface %s fetches %s:\n%s\n", getDescriptor().c_str(), url.c_str(), statusPage.c_str());
	}

