#ifndef HAVE_INTERFACE_FRIDGE_H
#define HAVE_INTERFACE_FRIDGE_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "out.h"
#include "stringStore.h"

using namespace std;

// Base class for interfaces. An interface is responsible for acquiring the values/datas for in's. In the future also for setting out's. This is the base class, every in should be an expansion of this. Dont know if this one should be instantiated as well, but should be possible for testing purposes. JCE, 20-6-13

class interface_fridge : public interface{
	public:
		interface_fridge(const string, const string, const string); // descr, name, ip-as-string
		~interface_fridge();
		void getIns();
		in *requests, *resets, *scanrate, *uptime, *latency;
		in *CO2;
		out *DC, *R0, *R1, *R2;
		in *temp, *rh, *dewp, *SOt, *SOrh;
		void setOut(out*, float);
	private:
		const string _ipstr;
	};

#endif // HAVE_INTERFACE_FRIDGE_H
