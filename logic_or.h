#ifndef HAVE_OR_H
#define HAVE_OR_H

#include "stdio.h"
#include <string>
#include "stringStore.h"
#include "out.h"
#include "outhost.h"
#include "pthread.h"
#include "logic.h"

using namespace std;

// Simple logic to glue together two outs. An out can have only a single input source, this is protected by PThreads. Still, two
// conflicting inputs an result in fast out switching. This is an intermediate out system, it sets the parameter out at creation
// if one of the two created outs are high. If both are low, the resulting out is also set low.
class logic_or: public logic {
	public:
		logic_or(const string, const string, out*); // bla, bla, measurement, actuator, "+" if actuator is incrementing the measurement, "-" if actuator is decrementing actuator, setpoint, difference setpoint and onTreshold, difference setpoint and offTreshold.
		~logic_or();
		void run(){}
		void setOut(out*, float);	// Intended for being called by out instances. JCE, 16-7-13
		int make_page(struct mg_connection*);
		out *A, *B;
	private:
		out *_myOut;
	};

#endif // HAVE_OR_H
