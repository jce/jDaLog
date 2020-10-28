#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_ping.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "out.h"

#define debug

using namespace std;

interface_ping::interface_ping(const string d, const string n, float i, const string ipstr):interface(d, n, i), _ipstr(ipstr){
	//in *requests, *resets, *scanrate, *uptime, *latency;
	latency = new in(getDescriptor() + "_lt", getName() + " latency", "ms", 3);
	}

interface_ping::~interface_ping(){
	delete latency;
	}

void interface_ping::getIns(){
	double start = now();
	string powerPage = get_html_page(_ipstr, 2);
	double end = now();
	double t = (start + end) / 2;
	latency->setValue((end-start)*1000/2, t); // Apparently it takes two round trips for HTTP. http://www.cloudping.info/ JCE, 10-8-2015
	//printf("\npage read:\n");
	//printf(powerPage.c_str());
	}

void interface_ping::setOut(out*, float){
	}
