#ifndef HAVE_INTERFACE_XMRSTAK_H
#define HAVE_INTERFACE_XMRSTAK_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "webin.h"
#include "stringStore.h"

using namespace std;


class interface_xmrstak : public interface{
	public:
		interface_xmrstak(const string, const string, float, const string); // descr, name, ip-as-string
		~interface_xmrstak();
		void getIns();
		in *hashrate, *runtime, *powerUsed, *latency;
		webin *powerConsumption;
	private:
		const string _ipstr;
		double prevT;	
	};

#endif // HAVE_INTERFACE_XMRSTAK_H
