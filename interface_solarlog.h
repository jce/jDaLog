#ifndef HAVE_INTERFACE_SL_H
#define HAVE_INTERFACE_SL_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "stringStore.h"
#include "modbus/modbus.h"

using namespace std;

// Base class for interfaces. An interface is responsible for acquiring the values/datas for in's. In the future also for setting out's. This is the base class, every in should be an expansion of this. Dont know if this one should be instantiated as well, but should be possible for testing purposes. JCE, 20-6-13

class interface_solarlog : public interface{
	public:
		interface_solarlog(const string, const string, const string); // descr, name, ip-as-string
		~interface_solarlog();
		void getIns();
		void makeModbusContext();
		//in *temp, *rh, *dewp, *SOt, *SOrh, *requests, *resets, *scanrate, *uptime, *latency;
		in *Pac, *Pdc, *Uac, *Udc, *efficiency, *totalKwhProduced, *latency;
	private:
		const string _ipstr;
		modbus_t *_ctx;
		double _lastValidDateTime;
	};

#endif // HAVE_INTERFACE_SL_H
