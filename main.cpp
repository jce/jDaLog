#include "stdio.h"
#include <list>
#include <string>
#include "string.h"
#include "floatLog.h"
#include "in_to_maria.h"
#include "main.h"
#include "mongoose.h"
#include "version.h"
#include "webgui.h"
#include "webgui2.h"
#include "webgui_mac.h"
#include "sys/time.h"
#include "unistd.h" // usleep
#include "pthread.h"
#include "math.h" // fmod(a, b);
#include "in.h" 
#include "interface.h"
#include "interface_rwl.h"
#include "interface_host.h"
#include "interface_solarlog.h"
#include "mytime.h" // now()
#include "mysql/mysql.h"
#include "interface_adam6052.h"
#include "interface_S1200.h"
#include "interface_macp.h"
#include "webin.h"
//#include "interface_xmrstak.h"
#include <signal.h>
#include "interface_darksky.h"
#include <curl/curl.h>
#include "interface_maria.h"
#include "interface_fijnstof.h"
#include "logic_fijnstof.h"
#include "logic_rain.h"
#include "interface_hs110.h"
#include "logic_power.h"
#include "jansson.h"

using namespace std;
//#define debug

// The main.cpp file. This file serves as incubator for new additions / components. JCE, 17-6-13

bool run(true);
bool globalControl(true);	// The inverse of isDevelopmentVersion heh. JCE, 25-9-13

interface *rwl;
interface *host, *solarlog, *S1200, *darksky, *dsAalsmeer, *dsOosterend;
in *buildNr, *version, *haveControl;
interface *scan_xiaomi;
interface *xmrstak_main;
interface *maria, *fijnstof;
logic *lfijnstof, *lrain;
interface *hs110_werkplaats, *hs110_koelkasten, *hs110_kamer;
logic *lpower;
in *pwrsum, *kWhsum;

// Signal handling.
void handle_signal(int signal){
	switch (signal){
		case SIGHUP:
		case SIGTERM:
		case SIGINT:
			run=false;
		};
	};

struct myThread{
	pthread_t thread;
	void (*func)();
	float interval;
	//in *scantime;
	string name;
	};

list<myThread*> myThreadList;

void* myThreadFunc(void* blah){
	struct myThread* data = (myThread*) blah;
	printf("%f second loop has started\n", data->interval);
	double start, end;
	char sname[101], lname[101];
	if (data->name != "")
	{
		snprintf(sname, 100,  "scant%s_%fs", data->name.c_str(), data->interval);
		snprintf(lname, 100, "Scantime %s %f second scan", data->name.c_str(), data->interval);
	}
	else
	{
		snprintf(sname, 100,  "scant%fs", data->interval);
		snprintf(lname, 100, "Scantime %f second scan", data->interval);
	}
	in scantime(sname, lname, "s", 6);
	double ttni = 0;
	while (run){
		if (ttni == 0){
			start = now();	// set start time
			#ifdef debug
				printf("<%f s loop: %f s", data->interval, start);
			#endif
			data->func();
			end = now();
			#ifdef debug
				printf("%f s loop>\n", data->interval);
			#endif
			scantime.setValue(end-start);}
		#ifdef debug
			//printf("<%u thinking how long to sleep>\n", data->interval);
		#endif
		ttni = data->interval - fmod(now(), data->interval);
		if (ttni > 0.1){
			#ifdef debug
				//printf("<%usleep0.1s>\n", data->interval);
			#endif
			usleep(100000);
			ttni -= 0.1;}
		else{
			#ifdef debug
				//printf("<%usleep%fs>\n", data->interval, ttni);
			#endif
			usleep(ttni * 1000000);
			ttni = 0;}
		#ifdef debug
			//printf("<%uwoke>\n", data->interval);
		#endif
		}
	printf("%f second loop has stopped\n", data->interval);
	return NULL;}

void callFuncOnInterval(void(*func)(), float interval, string name = ""){
	myThread *t = new myThread;
	t->func = func;
	t->interval = interval;
	t->name = name;	

	pthread_create(&(t->thread), NULL, &myThreadFunc, (void*) t);
	myThreadList.push_back(t);
	}

void loop1s(){
	S1200->getIns();

	// Manually calculate the sum of the three usage trackers/counters. JCE, 2-10-2020
	in *hs110_rm_p = get_in("hs110_rm_p");
	in *hs110_fr_p = get_in("hs110_fr_p");
	in *hs110_wp_p = get_in("hs110_wp_p");
	if (hs110_rm_p and hs110_fr_p and hs110_wp_p)
		pwrsum->setValue(hs110_rm_p->getValue() + hs110_fr_p->getValue() + hs110_wp_p->getValue());
	else
		pwrsum->setValid(false);

	// Manually calculate the sum of the three usage trackers/counters. JCE, 2-10-2020
	in *hs110_rm_tot = get_in("hs110_rm_tot");
	in *hs110_fr_tot = get_in("hs110_fr_tot");
	in *hs110_wp_tot = get_in("hs110_wp_tot");
	if (hs110_rm_tot and hs110_fr_tot and hs110_wp_tot)
		kWhsum->setValue(hs110_rm_tot->getValue() + hs110_fr_tot->getValue() + hs110_wp_tot->getValue());
	else
		kWhsum->setValid(false);
	}

void loop_hs110_kamer()
{
	hs110_kamer->getIns();
}
void loop_hs110_werkplaats()
{
	hs110_werkplaats->getIns();
}
void loop_hs110_koelkasten()
{
	hs110_koelkasten->getIns();
}

void loop10s(){
	rwl->getIns();
	host->getIns();
	//hs110_werkplaats->getIns();
	}

void loop11s()
{
	fijnstof->getIns();
}

void loopDarksky(){
	darksky->getIns();
	dsAalsmeer->getIns();
	dsOosterend->getIns();
	}

void loop15s(){
	solarlog->getIns();}

void loop60s(){
	buildNr->setValue(tcBuildNr);
	version->setValue(tcProgramVersion);
	haveControl->setValue(globalControl);
	touchAllWebins();
	// scan_xiaomi->getIns();
	//xmrstak_main->getIns();
	scan_xiaomi->getIns();
}

void loopstoreio(){
	// for all esnsors, call store to file
	map<string, in*>::iterator i;
	for (i = inmap.begin(); i!= inmap.end(); i++){
		i->second->importData();
		i->second->writeToFile();}
	// webgui.h / webgui.cp
	deleteOldFiles(); // AKA segmentation fault... JCE, 5-7-13
	}

void loop_in_to_maria()
{
	sleep(2);
	in_to_maria();
}

int main(){
	// Signal handler
	struct sigaction sa;
	sa.sa_handler = &handle_signal;
	sa.sa_flags = SA_RESTART;
	sigfillset(&sa.sa_mask);
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	// According to Curl docs:
	// https://curl.haxx.se/libcurl/c/curl_global_init.html
	// JCE, 25-9-2018
	curl_global_init(CURL_GLOBAL_ALL);
	mysql_library_init(0, NULL, NULL);

	// Startup
	printf("welkom bij %s V %.3f build %i\n", tcProgramName, tcProgramVersion, (int)tcBuildNr);

	// Read configuration
    json_t *json;
    json_error_t error;
    FILE *fp = NULL;
    fp = fopen("config.json", "r");
    if (!fp)
    {
        printf("config.json missing.\n");
        exit(0);
    }
    json = json_loadf(fp, 0, &error);
    fclose(fp);
    if (not json)
    {
        printf("interpreting config.json failed: %s at position %i (line %i, column %i)\n", error.text, error.position, error.line, error.column);
        return(0);
    }


	buildNr = new in("buildnr", "Build nummer", ""); //buildNr.setValue(tcBuildNr);
	version = new in("progver", "Program version", "", 3); //version.setValue(tcProgramVersion);
	rwl = new interface_rwl("rwl", "Regen waterlevel interface", "10.0.0.23");
	host = new interface_host("host", "Host");
	solarlog = new interface_solarlog("solarlog", "Solarlog", "10.0.0.4");
	haveControl = new in("prog_ctrl", "Program has control", "");
	S1200 = new interface_S1200("S1200", "S1200", "10.0.1.10");
	darksky = new interface_darksky("darksky", "Darksky", "a429dc31f36cda5cf90d27b562cb2325", 52.00788, 4.57637);
	dsAalsmeer = new interface_darksky("dsaalsmeer", "DS Aalsmeer", "e9f1b5d9e3a499a00b7c2d85b62ca01f", 52.27589, 4.77275);
	dsOosterend = new interface_darksky("dsoosterend", "DS Oosterend", "1bb2281b8e17ee1e94051a13d9edbe5b", 53.40416, 5.37819);
	//scan_xiaomi = new interface_macp("xp", "Scan Xiaomi", "64:09:80:c7:3f:4e");
	scan_xiaomi = new interface_macp("xp", "Scan Xiaomi", "20:47:da:20:b6:fb");
	//xmrstak_main = new interface_xmrstak("main", "main", "10.0.0.40:16000");
	maria = new interface_maria("maria", "Maria");
	fijnstof = new interface_fijnstof("fijnstof", "Fijnstof", "10.0.0.139");
	lfijnstof = new logic_fijnstof("lfijnstof", "Lfijnstof");
	lrain = new logic_rain("lrain", "LRain");
	hs110_werkplaats = new interface_hs110("hs110_wp", "Werkplaats", "10.10.0.1");
	hs110_koelkasten = new interface_hs110("hs110_fr", "Koelkasten", "10.10.0.2");
	hs110_kamer = new interface_hs110("hs110_rm", "Kamer", "10.10.0.3");
	lpower = new logic_power("lpower", "LPower");
	pwrsum = new in("pwrsum", "Power sum", "W", 3);
	kWhsum = new in("kwhsum", "kWh sum", "kWh", 3);

	if (not globalControl)
		webGuiStart("8094");
	else
		webGuiStart();
	
	callFuncOnInterval(loop1s, 1);
	callFuncOnInterval(loop_hs110_kamer, 1, "kamer");
	callFuncOnInterval(loop_hs110_werkplaats, 1, "werkplaats");
	callFuncOnInterval(loop_hs110_koelkasten, 1, "koelkaten");
	callFuncOnInterval(loop10s, 10);
	callFuncOnInterval(loop11s, 11);
	callFuncOnInterval(loop15s, 15); // only this: keeps running
	callFuncOnInterval(loop60s, 60); // only this: keeps running	
	callFuncOnInterval(loopstoreio, 1*3600); // only this: keeps running
	callFuncOnInterval(loopDarksky, (24.0*3600.0)/1000.0); // 1000 calls per day
	//callFuncOnInterval(loop_in_to_maria, 3600);

	usleep(100000);
	printf("running... (press control+C to stop)\n");
	//(void) getc (stdin);
	// Wait for signal...
	while(run){sleep(1000);}
	printf("shuttind down...\n");
	run = false;
	//webGuiStop();
	
	// for all threads, join
	list<myThread*>::iterator i;
	if (myThreadList.size())
		for (i = myThreadList.begin(); i != myThreadList.end(); i++)
			pthread_join((*i)->thread, NULL);

	delete kWhsum;
	delete pwrsum;
	delete lpower;
	delete hs110_kamer;
	delete hs110_koelkasten;
	delete hs110_werkplaats;
	delete lrain;
	delete lfijnstof;
	delete fijnstof;
	delete maria;

	//delete xmrstak_main;
	delete scan_xiaomi;
	delete darksky;
	delete dsAalsmeer;
	delete dsOosterend;
	delete S1200;
	delete rwl;
	delete host;
	delete solarlog;
	delete buildNr;
	delete version;
	delete haveControl;

	mysql_library_end();

	printf("%s V %.3f build %i has shutdown. byebye.\n", tcProgramName, tcProgramVersion, (int)tcBuildNr);
	return 0;
	}

