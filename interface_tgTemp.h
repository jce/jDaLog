#ifndef HAVE_INTERFACE_TGTEMP_H
#define HAVE_INTERFACE_TGTEMP_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "stringStore.h"

using namespace std;

// Base class for interfaces. An interface is responsible for acquiring the values/datas for in's. In the future also for setting out's. This is the base class, every in should be an expansion of this. Dont know if this one should be instantiated as well, but should be possible for testing purposes. JCE, 20-6-13

class interface_tgTemp : public interface{
	public:
		interface_tgTemp(const string, const string, const string); // descr, name, ip-as-string
		~interface_tgTemp();
		void getIns();
		in *temp, *rh, *dewp, *SOt, *SOrh, *requests, *resets, *scanrate, *uptime, *latency;
	private:
		const string _ipstr;
	};

#endif // HAVE_INTERFACE_TGTEMP_H
