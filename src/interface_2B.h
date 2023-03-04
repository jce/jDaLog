#ifndef HAVE_INTERFACE_2B_H
#define HAVE_INTERFACE_2B_H


#define byte byte_override
#include "snap7.h"
#undef byte

#include "in.h"
#include "interface.h"
#include "out.h"
#include "stringStore.h"

#include "stdio.h"
#include <string>

//using namespace std;

// Base class for interfaces. An interface is responsible for acquiring the values/datas for in's. In the future also for setting out's. This is the base class, every in should be an expansion of this. Dont know if this one should be instantiated as well, but should be possible for testing purposes. JCE, 20-6-13

//typedef unsigned int S7Object;

class interface_2B : public interface{
	public:
		interface_2B(const std::string, const std::string, float, const std::string); // descr, name, ip-as-string
		~interface_2B();
		void getIns();

		std::map<std::string, in*> ins;
		//map<out*> outs;
		//map<webin*> webins;

		void setOut(out*, float);
	private:
		const std::string _ipstr;
        S7Object PLC;
		uint32_t scancounter, writecounter;
		double lastReadTime;
		float lastScanTime;
		int interaction_counter = 0;	// For generating a 30-interactions-pulse.
		bool interaction_30 = false;	// True once every 30 interactions.
	};

#endif // HAVE_INTERFACE_2B_H
