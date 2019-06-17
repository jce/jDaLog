#ifndef HAVE_OUT_H
#define HAVE_OUT_H

#include "stdio.h"
#include <string>
#include "floatLog.h"
//#include "interface.h"
#include "stringStore.h"
#include <map>
#include "in.h"
#include "pthread.h"

using namespace std;

// The In has a separate name and descriptor. The second is used for file identification, so a namechange wont result in losing all measurement data. JCE, 20-6-13

class out: public in {	// Een out is een in, maar dan aangepast. Namelijk er moet een setter bij. setout ofzo...
	public:
		//in(const string); // descr
		out(const string, const string name, const string units, const unsigned int decimals, void *in, const float min = 0, const float step = 1, const float max = 1); // descr, name, units, decimals in floating point representation.
		~out();
		float getMin();
		float getStep();
		float getMax();
		void setOut(float); 
		float getOut();
		void setManOut(float);
		float getManOut();
		void setMan(bool);
		bool getMan();
		bool getControl();
	private:
		void *_interface;	// this is an interface*, that has the setOut(out*, float) function. Cannot mutually include.
		bool _man, _control;			// man: in manual mode. manOut wordt op de uitgang gezet. control: of uberhaupt op de uitgang gezet wordt. ~true na 1 keer setOut.
		float _min, _step, _max, _out, _manOut; // control minimum, step size and maximum. Memory for out setpoint, and manual out setpoint.
		void _setout();
		pthread_mutex_t _mutex;
};

extern map<string, out*> outmap;
unsigned int outsInManual();

#endif // HAVE_OUT_H
