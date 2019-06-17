#ifndef HAVE_INTERFACE_PING_H
#define HAVE_INTERFACE_PING_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "out.h"
#include "stringStore.h"

using namespace std;

class interface_ping : public interface{
	public:
		interface_ping(const string, const string, const string); // descr, name, url
		~interface_ping();
		void getIns();
		in *latency;
		void setOut(out*, float);
	private:
		const string _ipstr;
	};

#endif // HAVE_INTERFACE_PING_H
