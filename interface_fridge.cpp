#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_fridge.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "out.h"

#define debug

using namespace std;

interface_fridge::interface_fridge(const string d, const string n, float i, const string ipstr):interface(d, n, i), _ipstr(ipstr){
	//in *requests, *resets, *scanrate, *uptime, *latency;
	requests = new in(getDescriptor() + "_rq", getName() + " request counter", "");
	resets = new in(getDescriptor() + "_rs", getName() + " reset counter", "");
	scanrate = new in(getDescriptor() + "_sr", getName() + " scanrate", "Hz");
	uptime = new in(getDescriptor() + "_ut", getName() + " uptime", "s");
	latency = new in(getDescriptor() + "_lt", getName() + " latency", "ms", 3);
	//in *DC, *R0, *R1, *R2;
	DC = new out(getDescriptor() + "_DC", getName() + " SSR dutycycle", "0-65535", 0, (void*)this, 0, 1, 65535);
	R0 = new out(getDescriptor() + "_R0", getName() + " Relay 0", "", 0, (void*)this);
	R1 = new out(getDescriptor() + "_R1", getName() + " Relay 1", "", 0, (void*)this);
	R2 = new out(getDescriptor() + "_R2", getName() + " Relay 2", "", 0, (void*)this);
	
	//in *C02
	CO2 = new in(getDescriptor() + "_co2", getName() + " CO2 level", "0-2");

	//in *temp, *rh;
	temp = new in(getDescriptor() + "_temp", getName() + " temperature", "degC", 3);
	rh = new in(getDescriptor() + "_rh", getName() + " humidity", "% RH", 2);
	dewp = new in(getDescriptor() + "_dewp", getName() + " dewpoint", "degC", 2);
	SOt = new in(getDescriptor() + "_SOt", getName() + " SOt", "");
	SOrh = new in(getDescriptor() + "_SOrh", getName() + " SOrh", "");
	}

interface_fridge::~interface_fridge(){
	delete requests;
	delete resets;
	delete scanrate;
	delete uptime;
	delete latency;
	delete DC;
	delete R0;
	delete R1;
	delete R2;
	delete CO2;
	delete temp;
	delete rh;
	delete dewp;
	delete SOt;
	delete SOrh;
	}

void interface_fridge::getIns(){
	double start = now();
	string powerPage = get_html_page(_ipstr + "/power.htm");
	string tempPage = get_html_page(_ipstr + "/tempmeas.htm");
	string co2Page = get_html_page(_ipstr + "/co2.htm");
	string debugPage = get_html_page(_ipstr + "/debug.htm");
	double end = now();
	double t = (start + end) / 2;
	latency->setValue((end-start)/4*1000, t); // 2 requests, scale = ms
	float f;

	// Power page
	if (findFloatAfter(powerPage, "SSR duty cycle is ", f)) 
		DC->setValue(f, t);
	else
		DC->setValid(false);
	
	if (strstr(powerPage.c_str(), "Cooling relay is  off")){
		R0->setValue(0, t);}
	else if (strstr(powerPage.c_str(), "Cooling relay is  on")){
		R0->setValue(1, t);}
	else
		R0->setValid(false);

	if (strstr(powerPage.c_str(), "Heating relay is  off")){
		R1->setValue(0, t);}
	else if (strstr(powerPage.c_str(), "Heating relay is  on")){
		R1->setValue(1, t);}
	else
		R1->setValid(false);

	if (strstr(powerPage.c_str(), "Master relay is  off")){
		R2->setValue(0, t);}
	else if (strstr(powerPage.c_str(), "Master relay is  on")){
		R2->setValue(1, t);}
	else
		R2->setValid(false);
	// Temperature page, not implemented at the moment...
	//
	
	// CO2 page
	if (findFloatAfter(co2Page, "CO2 level: ", f)) 
		CO2->setValue(f, t);
	else
		CO2->setValid(false);
	
	// Debug page
	if (findFloatAfter(debugPage, "Requests: ", f)) 
		requests->setValue(f, t);
	else
		requests->setValid(false);

	if (findFloatAfter(debugPage, "Resets: ", f)) 
		resets->setValue(f, t);
	else
		resets->setValid(false);

	if (findFloatAfter(debugPage, "Scans per second: ", f)) 
		scanrate->setValue(f, t);
	else
		scanrate->setValid(false);

	if (findFloatAfter(debugPage, "Uptime [s, d h:m:s]: ", f)) 
		uptime->setValue(f, t);
	else
		uptime->setValid(false);

	// Temperature page, copy pasted (wat slecht) from interface_tgTemp.cpp.
	if (findFloatAfter(tempPage, "SOt: ", f)) 
		SOt->setValue(f, t);
	else
		SOt->setValid(false);

	if (findFloatAfter(tempPage, "SOrh: ", f)) 
		SOrh->setValue(f, t);
	else
		SOrh->setValid(false);
	if(SOt->isValid() and SOrh->isValid()){
		// Calculations for temperature, relative humidity and dewpoint in accordance
		// to the sensirion SH11 datasheet
		// http://www.sensirion.com/fileadmin/user_upload/customers/sensirion/Dokumente/Humidity/Sensirion_Humidity_SHT1x_Datasheet_V5.pdf
		// JCE, 5-2-2013
		// Copied from python tempSensor3 script. JCE, 21-6-13

                // Temperature
		#define  d1 -39.638    // Substracted 0.038, interpolated between two datasheet values. JCE, 5-2-13
		#define d2 0.01
		float T = d1 + d2 * SOt->getValue();
		temp->setValue(T, t);

		// RHlinear
		#define c1 -2.0468
		#define c2 0.0367
		#define c3 -0.0000015955
		float RHlinear = c1 + c2 * SOrh->getValue() + c3 * pow(SOrh->getValue(), 2);

		// RHtrue
		#define t1 0.01
		#define t2 0.00008
		float RH = (T - 25) * (t1 + t2 * SOrh->getValue()) + RHlinear;
                rh->setValue(RH, t);

		// Dewpoint
		if (T >= -40 and T<= 50){
			float Tn, m, a, b, Td;	
			if (T < 0){
				Tn = 272.62;
				m = 22.46;}
			else{
				Tn = 243.12;
				m = 17.62;}
			if (RH > 0 and (Tn + T) > 0){
				a = log(RH / 100.0) + m * T / (Tn + T);
				b = m - log(RH / 100.0) - m * T / (Tn + T);
				if (b > 0){
					Td = Tn * a / b;
					dewp->setValue(Td, t);}
				else{
					dewp->setValid(false);}}
			else{
				dewp->setValid(false);}}
		else
			dewp->setValid(false);}
	else{
		temp->setValid(false);
		rh->setValid(false);
		dewp->setValid(false);}


	#ifdef debug
		//printf("Interface %s fetches %s:\n%s\n", getDescriptor().c_str(), url.c_str(), statusPage.c_str());
	#endif
	}

void interface_fridge::setOut(out* o, float v){
	if (!globalControl) return;
	char url[1001];
	if (v<0)v = 0;
	if (v>65535)v = 65535;
	unsigned int u = floor(v + 0.5);
	
	if (o == DC and u != DC->getValue()){
		snprintf(url, 1000, "%s/power.htm?ssrDC=%u", _ipstr.c_str(), u);get_html_page(url);
		getIns();}

	if (u > 1) u = 1;
	if (o == R0 and u != R0->getValue()){
		if (v == 1)
			{snprintf(url, 1000, "%s/power.htm?sr=cooling_relay_on", _ipstr.c_str());get_html_page(url);}
		else	
			{snprintf(url, 1000, "%s/power.htm?sr=cooling_relay_off", _ipstr.c_str());get_html_page(url);}
		getIns();}

	if (o == R1 and u != R1->getValue()){
		if (v == 1)
			{snprintf(url, 1000, "%s/power.htm?sr=heating_relay_on", _ipstr.c_str());get_html_page(url);}
		else	
			{snprintf(url, 1000, "%s/power.htm?sr=heating_relay_off", _ipstr.c_str());get_html_page(url);}
		getIns();}

	if (o == R2 and u != R2->getValue()){
		if (v == 1)
			{snprintf(url, 1000, "%s/power.htm?sr=master_relay_on", _ipstr.c_str());get_html_page(url);}
		else	
			{snprintf(url, 1000, "%s/power.htm?sr=master_relay_off", _ipstr.c_str());get_html_page(url);}
		getIns();}
	}
