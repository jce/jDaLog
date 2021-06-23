#ifndef HAVE_WEBIN_H
#define HAVE_WEBIN_H

#include "stdio.h"
#include <string>
#include "floatLog.h"
#include "stringStore.h"
#include <map>
#include "in.h"

#define WEBIN_STORE_INTERVAL	60	// Seconds

using namespace std;

// the special webin. Just like a normal in, but then originates from the http gui pages. More like a configuration... Will probobly result in crappy graphs?...
class webin: public in{
	public:
		webin(const string, const string name = "", const string units = "", const unsigned int decimals = 0); // descr, name, units, decimals in floating point representation.
		void setValue(float, double = 0.0);
		~webin();
	};

extern mutex webinmap_mutex;
extern std::map<std::string, webin*> webinmap;
webin* get_webin(const char*);
webin* get_webin(std::string);

// JCE, 16-9-13
void touchAllWebins(); // Touch all webins. Please run every minute.

#endif // HAVE_WEBIN_H
