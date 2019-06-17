#ifndef HAVE_INTERFACE_H
#define HAVE_INTERFACE_H

#include "stdio.h"
#include <string>
//#include "floatLog.h"
#include "stringStore.h"
#include "out.h"
#include "outhost.h"

using namespace std;

// Helper function(s) for children interfaces.
//

string get_html_page(const string&, long timeout = 1); // Fetches a webpage by cURL, and returns it as string
int findFloatAfter(const char*, const char*, float&); // Finds the float in string param1, directly after param2. Returns succes
int findFloatAfter(string, const char*, float&); // Same as above
double now(); // returns the current timestamp, with us resolution as a double.

// Base class for interfaces. An interface is responsible for acquiring the values/datas for in's. In the future also for setting out's. This is the base class, every in should be an expansion of this. Dont know if this one should be instantiated as well, but should be possible for testing purposes. JCE, 20-6-13

class interface: public outhost{
	public:
		interface(const string, const string name = ""); // descr, units, name
		virtual ~interface();
		virtual void getIns();
		virtual void setOuts();
		virtual void setOut(out*, float);	// Intended for being called by out instances. JCE, 16-7-13
		const string getDescriptor();
		const string getName();		// JCE, 20-6-13
		const string getNote();		// JCE, 20-6-13
	private:
		const string _descr;
		stringStore *_name, *_note; // JCE, 20-6-13
	};

extern map<string, interface*> interfaces;

#endif // HAVE_INTERFACE_H
