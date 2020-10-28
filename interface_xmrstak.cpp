#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_xmrstak.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include <regex>

using namespace std;

interface_xmrstak::interface_xmrstak(const string d, const string n, float i, const string ipstr):interface(d, n, i), _ipstr(ipstr){
		//in *remp, *rh, *dewp, *SOt, *SOrh, *requests, *resets, *scanrate, *uptime, *latency;
	hashrate = new in(getDescriptor() + "_hr", getName() + " hashrate", "H/s", 1);
	runtime = new in(getDescriptor() + "_rt", getName() + " runtime", "h", 1);
	powerUsed = new in(getDescriptor() + "_pc", getName() + " power used", "kWh", 1);
	powerConsumption = new webin(getDescriptor() + "_p", getName() + " power usage rate", "W");
	latency = new in(getDescriptor() + "_lt", getName() + " latency", "ms", 3);
	prevT = now();}

interface_xmrstak::~interface_xmrstak(){
	delete latency;
	delete powerConsumption;
	delete powerUsed;
	delete runtime;
	delete hashrate;}

void interface_xmrstak::getIns(){
	double start = now();
	string url = _ipstr + "/h";
	string page = get_html_page(url);
	double end = now();
	double t = (start + end) / 2;
	latency->setValue((end-start)*1000, t); // 2 requests, scale = ms
	float addedPower = 0;// [kWh]
	float addedRuntime = 0; //[h]
	float hr = 0; // [H/s]
	//printf("\r\n\r\n%s\r\n\r\n", page.c_str());
	
	regex hr60regex("([^\\. 0-9]+ *)([\\.0-9]+)([^\\. 0-9]+)([\\. 0-9]+)[^\\., 0-9]*Highest");
	smatch results;
	if (regex_search(page, results, hr60regex))
		{
		//	printf("\r\n\r\nyay results\r\n\r\n");
		//	printf("%i %s\r\n", results[0].length(), results[0].str().c_str());
		//	printf("%i %s\r\n", results[1].length(), results[1].str().c_str());
		//	printf("%i %s\r\n", results[2].length(), results[2].str().c_str());
		
		if (sscanf(results[2].str().c_str(), "%f", &hr) == 1)
			{
				addedRuntime = (t - prevT) / 3600.0;
				addedPower = addedRuntime * powerConsumption->getValue()/ 1000.0;
			}
		else
			hashrate->setValid(false);
		}
	else
		hashrate->setValid(false);

	prevT = now();


	hashrate->setValue(hr, t);
	runtime->setValue(runtime->getValue() + addedRuntime, t);
	powerUsed->setValue(powerUsed->getValue() + addedPower, t);
	/*float f;


	
	// Status page
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

	*/
	//printf("Interface %s fetches %s:\n%s\n", getDescriptor().c_str(), url.c_str(), statusPage.c_str());
	}

