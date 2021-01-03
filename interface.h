#ifndef HAVE_INTERFACE_H
#define HAVE_INTERFACE_H

#include "stdio.h"
#include <string>
#include "stringStore.h"
#include "in.h"
#include "main.h"
#include "out.h"
#include "outhost.h"


// Helper function(s) for children interfaces.
//

std::string get_html_page(const std::string&, long timeout = 1); // Fetches a webpage by cURL, and returns it as string
int findFloatAfter(const char*, const char*, float&); // Finds the float in string param1, directly after param2. Returns succes
int findFloatAfter(std::string, const char*, float&); // Same as above
void findFloatAfter(const char*, const char*, in*, double); // Finds the float in string param1, directly after param2. Writes value and success to in at timestamp double
void findFloatAfter(std::string, const char*, in*, double); // Same as above
//template<class T> int findVarAfter(string, const char*, T&); // Same as above
//int findDoubleAfter(const char*, const char*, double&); // Finds the float in string param1, directly after param2. Returns succes
//int findDoubleAfter(std::string, const char*, double&); // Same as above
int findIntAfter(const char*, const char*, int&); // Finds the float in string param1, directly after param2. Returns succes
int findIntAfter(std::string, const char*, int&); // Same as above
double now_mt();	// Fetches now on a the monotonic clock.

double now(); // returns the current timestamp, with us resolution as a double.

// Base class for interfaces. An interface is responsible for acquiring the values/datas for in's. In the future also for setting out's. This is the base class, every in should be an expansion of this. Dont know if this one should be instantiated as well, but should be possible for testing purposes. JCE, 20-6-13

class interface: public outhost{
	public:
		interface(const std::string, const std::string, float interval); // descr, units, name
		virtual ~interface();
		virtual void getIns();
		virtual void setOuts();
		virtual void setOut(out*, float);	// Intended for being called by out instances. JCE, 16-7-13
		const std::string getDescriptor();
		const std::string getName();		// JCE, 20-6-13
		const std::string getNote();		// JCE, 20-6-13
		virtual void start();	// Start the thread.  JCE, 28-10-2020
		virtual void stop();	// Signals the thread(s) to stop. JCE, 28-10-2020
		virtual void join();	// Joins the thread (waits untill it is really stopped). JCE, 28-10-2020
		virtual void run();		// Override in derived class.
	protected:
		bool run_flg;
	private:
		const std::string _descr;
		stringStore *_name, *_note; // JCE, 20-6-13
		pthread_t thread = 0;
		float interval; // [s] time between getIns calls.
	};

extern std::map<std::string, interface*> interfaces;

#endif // HAVE_INTERFACE_H
