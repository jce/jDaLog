#ifndef HAVE_ONOFFREGULATOR_H
#define HAVE_ONOFFREGULATOR_H

#include "stdio.h"
#include <string>
#include "stringStore.h"
#include "out.h"
#include "outhost.h"
#include "pthread.h"
#include "logic.h"

using namespace std;

class logic_onOffRegulator: public logic {
	public:
		logic_onOffRegulator(const string, const string, in*, out*, string, in* = NULL, in* = NULL, in* = NULL); // bla, bla, measurement, actuator, "+" if actuator is incrementing the measurement, "-" if actuator is decrementing actuator, setpoint, difference setpoint and onTreshold, difference setpoint and offTreshold.
		~logic_onOffRegulator();
		void run();
		void setOut(out*, float);	// Intended for being called by out instances. JCE, 16-7-13
		int make_page(struct mg_connection*);
	private:
		in *_in, *_low, *_high, *_sp, *_e;
		out *_out;
		string _dir;
		bool _myLow, _myHigh, _mySP;
	};

#endif // HAVE_ONOFFREGULATOR_H
