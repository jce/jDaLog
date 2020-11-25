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
#include "in_from_dir.h"
#include "logic_km.h"
#include "interface_pi_gpio.h"
#include "interface_sht3x.h"

using namespace std;
//#define debug

// The main.cpp file. This file serves as incubator for new additions / components. JCE, 17-6-13

bool run(true);
bool globalControl(true);	// The inverse of isDevelopmentVersion heh. JCE, 25-9-13

in *buildNr, *version, *haveControl;
logic *lfijnstof, *lrain;
logic *lpower;
in *pwrsum = NULL, *kWhsum = NULL;

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

void build_interfaces(json_t *arr)
{
	if (! json_is_array(arr))
		return;
	size_t index;
	json_t *json, *jscan, *jlon, *jlat, *ji2c_id;
	const char *type, *id, *name, *key, *address, *pingrange, *i2c_dev;
	float scan, lon, lat;
	uint8_t i2c_id;
	json_array_foreach(arr, index, json)
	{
		if (json_is_object(json))
		{
			// Get as many features as possible.
			type = 	json_string_value(json_object_get(json, "type"));
			if (type)
			{
				id = 	json_string_value(json_object_get(json, "id"));
				name = 	json_string_value(json_object_get(json, "name"));
				key = 	json_string_value(json_object_get(json, "key"));
				address = json_string_value(json_object_get(json, "address"));
				pingrange = json_string_value(json_object_get(json, "pingrange"));
				i2c_dev = json_string_value(json_object_get(json, "i2c_dev"));
				jlon =  json_object_get(json, "lon");
				lon = json_number_value(jlon);
				jlat =  json_object_get(json, "lat");
				lat = json_number_value(jlat);
				jscan =  json_object_get(json, "scan");
				scan = json_number_value(jscan);
				ji2c_id =  json_object_get(json, "i2c_id");
				i2c_id = json_integer_value(ji2c_id);

				// Build different types
				if (strcmp(type, "darksky") == 0)
				{
					if (id and name and key and json_is_number(jlon) and json_is_number(jlat) and json_is_number(jscan))
						new interface_darksky(id, name, scan, key, lon, lat);
					else
						printf("could not build interface_darksky(%s, %s, %f, %s, %f, %f)\n", id, name, scan, key, lon, lat);
				}
				if (strcmp(type, "solarlog") == 0)
				{
					if (id and name and json_is_number(jscan) and address)
						new interface_solarlog(id, name, scan, address);
					else
						printf("could not build interface_solarlog(%s, %s, %f, %s)\n", id, name, scan, address);
				}
				if (strcmp(type, "S1200") == 0)
				{
					if (id and name and json_is_number(jscan) and address)
						new interface_S1200(id, name, scan, address);
					else
						printf("could not build interface_S1200(%s, %s, %f, %s)\n", id, name, scan, address);
				}
				if (strcmp(type, "hs110") == 0)
				{
					if (id and name and json_is_number(jscan) and address)
						new interface_hs110(id, name, scan, address);
					else
						printf("could not build interface_hs110(%s, %s, %f, %s)\n", id, name, scan, address);
				}
				if (strcmp(type, "rwl") == 0)
				{
					if (id and name and json_is_number(jscan) and address)
						new interface_rwl(id, name, scan, address);
					else
						printf("could not build interface_rwl(%s, %s, %f, %s)\n", id, name, scan, address);
				}
				if (strcmp(type, "fijnstof") == 0)
				{
					if (id and name and json_is_number(jscan) and address)
						new interface_fijnstof(id, name, scan, address);
					else
						printf("could not build interface_fijnstof(%s, %s, %f, %s)\n", id, name, scan, address);
				}
				if (strcmp(type, "macp") == 0)
				{
					if (id and name and json_is_number(jscan) and address and pingrange)
						new interface_macp(id, name, scan, address, pingrange);
					else
						printf("could not build interface_macp(%s, %s, %f, %s, %s)\n", id, name, scan, address, pingrange);
				}
				if (strcmp(type, "host") == 0)
				{
					if (id and name and json_is_number(jscan))
					{
						interface_host *h = new interface_host(id, name, scan);
						json_t *disk_j;
						const char* path;
						json_object_foreach(json_object_get(json, "disks"), path, disk_j)
						{
							const char *did = json_string_value(json_object_get(disk_j, "id"));
							const char *dname = json_string_value(json_object_get(disk_j, "name"));
							h->add_disk(path, did, dname);
						}
					}
					else
						printf("could not build interface_host(%s, %s, %f)\n", id, name, scan);
				}
				if (strcmp(type, "pi_gpio") == 0)
				{
					if (id and name and json_is_number(jscan))
					{
						interface_pi_gpio *i = new interface_pi_gpio(id, name, scan);
						read_gpios_from_json(i, json_object_get(json, "gpio"));
					}
					else
						printf("could not build interface_pi_gpio(%s, %s, %f)\n", id, name, scan);
				}
				if (strcmp(type, "sht3x") == 0)
				{
					if (id and name and json_is_number(jscan) and i2c_dev and json_is_integer(ji2c_id))
						new interface_sht3x(id, name, scan, i2c_dev, i2c_id);
					else
						printf("could not build interface_sht3x(%s, %s, %f, %s, %d)\n", id, name, scan, i2c_dev, i2c_id);
				}
				if (strcmp(type, "maria") == 0)
				{
					if (id and name and json_is_number(jscan))
						new interface_maria(id, name, scan);
					else
						printf("could not build interface_maria(%s, %s, %f)\n", id, name, scan);
				}
			}
		}	
	}	
}

void build_dir_to_ins(json_t *arr)
{
	if (! json_is_array(arr))
		return;
	size_t index;
	json_t *json;
	const char *dir, *prefix;
	json_array_foreach(arr, index, json)
	{
		if (json_is_object(json))
		{
			dir = 	json_string_value(json_object_get(json, "dir"));
			prefix =json_string_value(json_object_get(json, "prefix"));
			read_ins_from_dir(dir, prefix);
		}
	}
}

void build_logics(json_t *arr)
{
	if (! json_is_array(arr))
		return;
	size_t index;
	json_t *json;
	in *inp;
	const char *type, *indescr, *name, *id;
	json_array_foreach(arr, index, json)
	{
		if (json_is_object(json))
		{
			type = 	json_string_value(json_object_get(json, "type"));
			if (type)
			{
				id = 	json_string_value(json_object_get(json, "id"));
				name = 	json_string_value(json_object_get(json, "name"));
				indescr = json_string_value(json_object_get(json, "in"));

				// Build different types
				if (strcmp(type, "km") == 0)
				{
					inp = get_in(indescr);
					if (id and name and inp)
						new logic_km(id, name, inp);
					else
						printf("could not build logic_km(%s, %s, get_in(%s))\n", id, name, indescr);
				}
			}
		}
	}
}

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

	// Manually calculate the sum of the three usage trackers/counters. JCE, 2-10-2020
	in *hs110_rm_p = get_in("hs110_rm_p");
	in *hs110_fr_p = get_in("hs110_fr_p");
	in *hs110_wp_p = get_in("hs110_wp_p");
	if (hs110_rm_p and hs110_fr_p and hs110_wp_p)
	{
		if (!pwrsum)
			pwrsum = new in("pwrsum", "Power sum", "W", 3);
		pwrsum->setValue(hs110_rm_p->getValue() + hs110_fr_p->getValue() + hs110_wp_p->getValue());
	}
	else
		if(pwrsum)
			pwrsum->setValid(false);

	// Manually calculate the sum of the three usage trackers/counters. JCE, 2-10-2020
	in *hs110_rm_tot = get_in("hs110_rm_tot");
	in *hs110_fr_tot = get_in("hs110_fr_tot");
	in *hs110_wp_tot = get_in("hs110_wp_tot");
	if (hs110_rm_tot and hs110_fr_tot and hs110_wp_tot)
	{
		if (!kWhsum)
			kWhsum = new in("kwhsum", "kWh sum", "kWh", 3);
		kWhsum->setValue(hs110_rm_tot->getValue() + hs110_fr_tot->getValue() + hs110_wp_tot->getValue());
	}
	else
		if(pwrsum)
			kWhsum->setValid(false);
	}

void loop60s(){
	buildNr->setValue(tcBuildNr);
	version->setValue(tcProgramVersion);
	haveControl->setValue(globalControl);
	touchAllWebins();
	// scan_xiaomi->getIns();
	//xmrstak_main->getIns();
	//scan_xiaomi->getIns();
}

void loopstoreio(){
	// for all esnsors, call store to file
	map<string, in*>::iterator i;
	for (i = inmap.begin(); i!= inmap.end(); i++)
	{
		i->second->importData();
		i->second->writeToFile();
	}
	// webgui.h / webgui.cp
	deleteOldFiles(); // AKA segmentation fault... JCE, 5-7-13
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

	build_interfaces(json_object_get(json, "interface"));
	build_dir_to_ins(json_object_get(json, "dir_to_ins"));
	build_logics(json_object_get(json, "logic"));
	out_conf(json_object_get(json, "out"));
	
	//uint16_t port = 8090;
	json_t *webgui_j = json_object_get(json, "webgui");
	if (json_is_object(webgui_j))
	{
		json_t *def_w_j, *def_h_j;// *port_j
		//port_j = json_object_get(webgui_j, "port");
		def_w_j = json_object_get(webgui_j, "def_w");
		def_h_j = json_object_get(webgui_j, "def_h");
		//if (json_is_integer(port_j))
		//	port = json_integer_value(port_j);
		if (json_is_integer(def_w_j))
			def_w = json_integer_value(def_w_j);
		if (json_is_integer(def_h_j))
			def_h = json_integer_value(def_h_j);
	}

	buildNr = new in("buildnr", "Build nummer", ""); //buildNr.setValue(tcBuildNr);
	version = new in("progver", "Program version", "", 3); //version.setValue(tcProgramVersion);
	haveControl = new in("prog_ctrl", "Program has control", "");
	lfijnstof = new logic_fijnstof("lfijnstof", "Lfijnstof");
	lrain = new logic_rain("lrain", "LRain");
	lpower = new logic_power("lpower", "LPower");
	//pwrsum = new in("pwrsum", "Power sum", "W", 3);
	//kWhsum = new in("kwhsum", "kWh sum", "kWh", 3);

	if (not globalControl)
		webGuiStart("8094");
	else
		webGuiStart();
	
	for(map<string, interface*>::iterator i = interfaces.begin(); i != interfaces.end(); i++)
		i->second->start();
	callFuncOnInterval(loop1s, 1);
	callFuncOnInterval(loop60s, 60); // only this: keeps running	
	callFuncOnInterval(loopstoreio, 1*3600); // only this: keeps running

	usleep(100000);
	printf("running... (press control+C to stop)\n");
	//(void) getc (stdin);
	// Wait for signal...
	while(run){sleep(1000);}
	printf("shuttind down...\n");
	run = false;
	//webGuiStop();

	for(map<string, interface*>::iterator i = interfaces.begin(); i != interfaces.end(); i++)
		i->second->stop();
	
	// for all threads, join
	list<myThread*>::iterator i;
	if (myThreadList.size())
		for (i = myThreadList.begin(); i != myThreadList.end(); i++)
			pthread_join((*i)->thread, NULL);

	for(map<string, interface*>::iterator i = interfaces.begin(); i != interfaces.end(); i++)
		i->second->join();

	for(map<string, interface*>::iterator i = interfaces.begin(); i != interfaces.end(); i++)
		delete i->second;
	
	if(kWhsum)
		delete kWhsum;
	if (pwrsum)
		delete pwrsum;
	delete lpower;
	delete lrain;
	delete lfijnstof;
	delete buildNr;
	delete version;
	delete haveControl;

	mysql_library_end();

	printf("%s V %.3f build %i has shutdown. byebye.\n", tcProgramName, tcProgramVersion, (int)tcBuildNr);
	return 0;
	}

