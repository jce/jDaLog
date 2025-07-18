#include "alloca.h"
#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_dcmr.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "sys/statvfs.h" // statvfs()
#include "sys/resource.h" // getrusage
#include <sys/types.h> // getpid()
#include <unistd.h> // getpid() // sysconf
#include <stdint.h>
#include "unistd.h" // sysconf()
#include <unistd.h> // exec(), JCE, 3-9-2018
#include <iostream>
#include <cstdio>
#include <memory>
#include <curl/curl.h>

//#define DBG(...)
#define DBG(...) { printf(__VA_ARGS__); printf("\n"); }

using namespace std;

// Recycled from darksky interface
static size_t writeCallback(void* buf, size_t size, size_t nmemb, void* userp)
{
	if(userp)
	{
		string* s = static_cast<string*>(userp);
		size_t len = size * nmemb;
		s->append(static_cast<char*>(buf), len);
		return nmemb;
	}

	return 0;
}

string get_https_page(const string& url, long timeout) // Make sure https is in the url. JCE, 19-9-2018
{
	CURL* curl = curl_easy_init();

	string s;

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_FILE, &s);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	// curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Do not verify the host
	// curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // Do not verify hostname on certificate
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	return s;
}

interface_dcmr::interface_dcmr(const string d, const string n, float i):interface(d, n, i), _prevT(0)
{
	boslaan =	 		new in(getDescriptor() + "_bl", getName() + " Boslaan", "dB", 0);
	achterdijk = 		new in(getDescriptor() + "_ad", getName() + " Achterdijk", "dB", 0);
	veldkersweg	= 		new in(getDescriptor() + "_vw", getName() + " Veldkersweg", "dB", 0);
	pyramide = 			new in(getDescriptor() + "_py", getName() + " Pyramide", "dB", 0);
	vosmaerstraat = 	new in(getDescriptor() + "_vs", getName() + " Vosmaerstraat", "dB", 0);
	havikenhof = 		new in(getDescriptor() + "_hh", getName() + " Havikenhof", "dB", 0);
	canniuslaan = 		new in(getDescriptor() + "_cl", getName() + " Pastoor Canniuslaan", "dB", 0);
	rubenslaan = 		new in(getDescriptor() + "_rl", getName() + " Rubenslaan", "dB", 0);
	uiverstraat = 		new in(getDescriptor() + "_us", getName() + " Uiverstraat", "dB", 0);
	develden = 			new in(getDescriptor() + "_dv", getName() + " De Velden", "dB", 0);
	kasteelweg = 		new in(getDescriptor() + "_kw", getName() + " Kasteelweg", "dB", 0);
	nachtegaallaan = 	new in(getDescriptor() + "_nl", getName() + " Nachtegaallaan", "dB", 0);
	kruislaan = 		new in(getDescriptor() + "_kl", getName() + " Rode Kruislaan", "dB", 0);
	kareldegrotelaan = 	new in(getDescriptor() + "_dl", getName() + " Karel de Grotelaan", "dB", 0);

	head = boslaan->getTime();
	if (head < 1718170000)
		head = 1718170000;
}

interface_dcmr::~interface_dcmr()
{
	delete boslaan;
	delete achterdijk;
	delete veldkersweg;
	delete pyramide;
	delete vosmaerstraat;
	delete havikenhof;
	delete canniuslaan;
	delete rubenslaan;
	delete uiverstraat;
	delete develden;
	delete kasteelweg;
	delete nachtegaallaan;	
	delete kruislaan;
	delete kareldegrotelaan;
}

void id_to_in(json_t *json_id, int id, in *inp, double head)
{
	size_t idx2;
	json_t *level, *idfield, *levels;
	idfield = json_object_get(json_id, "id");
	if( idfield and json_number_value(idfield) == id)
	{
		// Catch, this is the correct id block
		levels = json_object_get(json_id, "levels");
		json_array_foreach(levels, idx2, level)
			if (json_array_size(level) == 4)
			{
				double t = json_number_value(json_array_get(level, 0)) + uint64_t(head / 60) * 60;
				double v = json_number_value(json_array_get(level, 3));
				//DBG("t = %f, v = %f", t, v);
				inp->setVal(v, t);
			}
	}

}

void interface_dcmr::run()
{
	while ( run_flg ) 
	{
		double thisT = now();
		double dt = thisT - head;
		if (dt < 3600)
		{
			sleep(1);
			continue;
		}
		if (dt > 3600)
			dt = 3600;

		double urlfrom = 1000 * head, urlto = 1000 * (head + dt);
		string url = "https://rtm.flighttracking.casper.aero/?/timeline&urls[0]=%2Fnmt%2Fupdates%2F&from=" + to_string(urlfrom) + "&upto=" + to_string(urlto);
		DBG("%s", url.c_str());
		string page = get_https_page(url, interval);
		//DBG("Page size: %ld", page.size());
		json_t *json = json_loads(page.c_str(), 0, NULL);

		// Lets find the json data
		json_t *json_id;
		size_t idx;
		json_array_foreach(json_object_get(json, "/nmt/updates/"), idx, json_id)
		{
			id_to_in(json_id, 1, kasteelweg, head);
			id_to_in(json_id, 2, nachtegaallaan, head);
			id_to_in(json_id, 3, achterdijk, head);
			id_to_in(json_id, 4, vosmaerstraat, head);
			id_to_in(json_id, 5, veldkersweg, head);
			id_to_in(json_id, 6, canniuslaan, head);
			id_to_in(json_id, 18, pyramide, head);
			id_to_in(json_id, 21, boslaan, head);
			id_to_in(json_id, 22, kruislaan, head);
			id_to_in(json_id, 23, uiverstraat, head);
			id_to_in(json_id, 24, kareldegrotelaan, head);
			id_to_in(json_id, 25, havikenhof, head);
			id_to_in(json_id, 26, develden, head);
			id_to_in(json_id, 27, rubenslaan, head);
		}
		json_decref(json);

		head += dt;
	}
}







