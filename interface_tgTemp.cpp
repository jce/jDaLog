#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_tgTemp.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()

using namespace std;

interface_tgTemp::interface_tgTemp(const string d, const string n, float i, const string ipstr):interface(d, n, i), _ipstr(ipstr){
		//in *remp, *rh, *dewp, *SOt, *SOrh, *requests, *resets, *scanrate, *uptime, *latency;
	temp = new in(getDescriptor() + "_temp", getName() + " temperature", "degC", 3);
	rh = new in(getDescriptor() + "_rh", getName() + " humidity", "% RH", 2);
	dewp = new in(getDescriptor() + "_dewp", getName() + " dewpoint", "degC", 2);
	SOt = new in(getDescriptor() + "_SOt", getName() + " SOt", "");
	SOrh = new in(getDescriptor() + "_SOrh", getName() + " SOrh", "");

	requests = new in(getDescriptor() + "_rq", getName() + " request counter", "");
	resets = new in(getDescriptor() + "_rs", getName() + " reset counter", "");
	scanrate = new in(getDescriptor() + "_sr", getName() + " scanrate", "Hz");
	uptime = new in(getDescriptor() + "_ut", getName() + " uptime", "s");
	latency = new in(getDescriptor() + "_lt", getName() + " latency", "ms", 3);}

interface_tgTemp::~interface_tgTemp(){
	delete temp;
	delete rh;
	delete dewp;
	delete SOt;
	delete SOrh;
	delete requests;
	delete resets;
	delete scanrate;
	delete uptime;
	delete latency;}

void interface_tgTemp::getIns(){
	double start = now();
	string url = _ipstr + "/temp.htm";
	string tempPage = get_html_page(url);
	string debugPage = get_html_page(_ipstr + "/debug.htm");
	double end = now();
	double t = (start + end) / 2;
	latency->setValue((end-start)/2*1000, t); // 2 requests, scale = ms
	float f;

	// Status page
	if (findFloatAfter(tempPage, "SOt: ", f)) 
		SOt->setValue(f, t);

	if (findFloatAfter(tempPage, "SOrh: ", f)) 
		SOrh->setValue(f, t);
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
			}
		}
	}

	// Debug page
	if (findFloatAfter(debugPage, "Requests: ", f)) 
		requests->setValue(f, t);

	if (findFloatAfter(debugPage, "Resets: ", f)) 
		resets->setValue(f, t);

	if (findFloatAfter(debugPage, "Scans per second: ", f)) 
		scanrate->setValue(f, t);

	if (findFloatAfter(debugPage, "Uptime [s, d h:m:s]: ", f)) 
		uptime->setValue(f, t);

	//printf("Interface %s fetches %s:\n%s\n", getDescriptor().c_str(), url.c_str(), statusPage.c_str());
	}

