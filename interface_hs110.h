#ifndef HAVE_INTERFACE_HS110_H
#define HAVE_INTERFACE_HS110_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "stringStore.h"
#include <netinet/in.h>
#include "float.h"

#define HS110_BUFSIZE 2048
using namespace std;

class interface_hs110 : public interface{
	public:
		interface_hs110(const string, const string, float, const string); // descr, name, ip-as-string
		~interface_hs110();
		void getIns();
		in *voltage, *current, *power, *total_kwh, *err_code, *latency, *va, *pf;
		in  *whreadout;
	private:
		const string _ipstr;
		char buf[HS110_BUFSIZE];
		struct sockaddr_in sa;
		float kWh_stored_at_startup;
		float Wh_hs110_at_startup = FLT_MAX;
		float Wh_hs110_last_readout = FLT_MAX;
	};

#endif // HAVE_INTERFACE_HS110_H
