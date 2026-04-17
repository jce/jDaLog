#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "main.h"
#include "math.h"	// fmod(a, b)
#include "unistd.h" // usleep
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "pthread.h"
#include "stringStore.h"
#include "out.h"

using namespace std;

// Helper functions, used in the children... JCE, 21-6-13
//

#include <curl/curl.h>

static size_t http_write(void* buf, size_t size, size_t nmemb, void* userp)
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

string get_html_page(const string& url, long timeout)
{
	CURL* curl = curl_easy_init();

	string s;

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &http_write);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_FILE, &s);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // Necessary, for multithreaded applications. Expect SIGPIPE... https://curl.haxx.se/libcurl/c/threadsafe.html JCE, 25-9-2018

	curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	return s;
}

// Simple finder functions are fine, no need for regex'es at the moment. JCE, 19-6-13
// Parameter 1 is the char* to search
// Parameter 2 is a char* containing the cue to search for
// Parameter 3 is a reference to fill with the corresponding thing found after the cue
// Return value should be 1 if succes.
int findFloatAfter(const char* str, const char* cue, float& f){
	const char* loc = strstr(str, cue);
	if (loc and strlen(loc) > strlen(cue))
		return sscanf(loc + strlen(cue), "%f", &f);
        return 0;}
int findFloatAfter(string str, const char* cue, float& f){
	return findFloatAfter(str.c_str(), cue, f);}

void findFloatAfter(const char* str, const char* cue, in *i, double t)
{
	float f;
	if (findFloatAfter(str, cue, f))
		i->setValue(f, t);
}

void findFloatAfter(string str, const char* cue, in *i, double t)
{
	findFloatAfter(str.c_str(), cue, i, t);
}

//int findDoubleAfter(const char* str, const char* cue, double& d){
//	const char* loc = strstr(str, cue);
//	if (loc and strlen(loc) > strlen(cue))
//		return sscanf(loc + strlen(cue), "%lf", &d);
//        return 0;}
//int findDoubleAfter(string str, const char* cue, double& d){
//	return findDoubleAfter(str.c_str(), cue, d);}

int findIntAfter(const char* str, const char* cue, int& i){
	const char* loc = strstr(str, cue);
	if (loc and strlen(loc) > strlen(cue))
		return sscanf(loc + strlen(cue), "%d", &i);
        return 0;}
int findIntAfter(string str, const char* cue, int& i){
	return findIntAfter(str.c_str(), cue, i);}

int findUnsignedLongLongAfter(const char* str, const char* cue, unsigned long long &l)
{
	const char* loc = strstr(str, cue);
	if (loc and strlen(loc) > strlen(cue))
		return sscanf(loc + strlen(cue), "%llu", &l);
    return 0;
}
int findUnsignedLongLongAfter(string str, const char* cue, unsigned long long &l)
{
	return findUnsignedLongLongAfter(str.c_str(), cue, l);
}

double now_mt()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double) ts.tv_sec + (double) ts.tv_nsec / 1000000000;
}

/*double now(){
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + (double) tv.tv_usec / 1000000;}*/

// Implementation of class in. In should give an uniform representation of inputs for tcFarmControl. I assume to use it later for both visualisation and as generic origin of information for internal logics. The in should also allow for "simple" enumeration and finding of any of the inputs. To be done by an in-map or list with search function. JCE, 19-6-13


map<string, interface*> interfaces;

interface::interface(const string d, const string n, float _interval): 
	interval(_interval),
	_descr(d) 
{
	string path = dataDir;	
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	path += "/interface/";
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	path += d;
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	//s += "/floatLog.bin";
	//_logger = new floatLog(path + "/floatLog.bin");
	//_units = new stringStore(path + "/units.txt");
	//if (_units->getString() != u and u != "") _units->setString(u);
	_name = new stringStore(path + "/name.txt");
	//if (_name->getString() != n and n != "") _name->setString(n); // Commented. JCE, 11-7-13, build 481
	_name->setString(n);
	_note = new stringStore(path + "/note.txt");
	//printf("eind init floatlog\n");
	interfaces[_descr] = this ;}
	//printf("eind in initializer\n");}

interface::~interface(){
	#ifdef debug
		printf("Deleting interface %s...\n", _descr.c_str());
	#endif
	//interfaces.erase(_descr);
	for(auto i = ins.begin(); i != ins.end(); i++)
		delete(i->second);

	_name->~stringStore();
	_note->~stringStore();
	#ifdef debug
		printf("Interface %s is deleted\n", _descr.c_str());
	#endif
	}
	//_logger->~floatLog();}
	//delete _logger;}

void interface::getIns(){
	printf("Interface %s is getting ins...\n", _descr.c_str());}

void interface::setOuts(){}

// Function intended for an out to call, if it needs to be set. The out itself cannot set the physical device, the interface has to do this.
void interface::setOut(out*, float){} // JCE, 16-7-13

const string interface::getDescriptor(){
	return _descr;}

const string interface::getName(){
	return _name->getString();}

const string interface::getNote(){
	return _note->getString();}

// Thread per interface
static void* interface_run_cc(void*);
void interface::start()
{
	run_flg = true;
	pthread_create(&thread, NULL, &interface_run_cc, (void*) this);
	char name[16];
	strncpy(name, _descr.c_str(), 15);
	name[15] = 0;
	pthread_setname_np(thread, name);
}

void interface::stop()
{
	run_flg = false;
}

void interface::join()
{
	stop();
	pthread_join(thread, NULL);
}

static void* interface_run_cc(void* p)
{
	interface *i = (interface*) p;
	i->run();
	return p;
}

void interface::run()
{
	float ttni = 0;
	while(run_flg)
	{	
		if (ttni <= 1)
		{
			getIns();
			ttni = 1000000.0 * (interval - fmod(now(), interval));
		}
		else if (ttni > 100000 )
		{
			ttni -= 100000;
			usleep(100000);
		}
		else
		{
			usleep(ttni);
			ttni = 0;
		}
	}
} 
