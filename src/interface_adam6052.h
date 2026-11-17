#ifndef HAVE_6052_H
#define HAVE_6052_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "stringStore.h"
#include "modbus/modbus.h"
#include "pthread.h"

using namespace std;

// Base class for interfaces. An interface is responsible for acquiring the values/datas for in's. In the future also for setting out's. This is the base class, every in should be an expansion of this. Dont know if this one should be instantiated as well, but should be possible for testing purposes. JCE, 20-6-13

class interface_adam6052 : public interface{
	public:
		interface_adam6052(const string, const string, float, const string); // descr, name, ip-as-string
		~interface_adam6052();
		void getIns();
		void makeModbusContext();
		void setOut(out*, float);
		in *latency;
		in *DI[8], *CI[8];//, *DOL[8], *DOH[8];
		out *DO[8];
	private:
		const string _ipstr;
		modbus_t *_ctx;
		// _ctx is used by both getIns() (interface thread) and setOut() (web thread);
		// libmodbus contexts are not thread-safe, so all use is serialized with this mutex.
		pthread_mutex_t _ctx_mutex = PTHREAD_MUTEX_INITIALIZER;
		double _lastValidDateTime;
	};

#endif // HAVE_6052_H
