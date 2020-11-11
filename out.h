#ifndef HAVE_OUT_H
#define HAVE_OUT_H

#include "out_expr.h"


#include "stdio.h"
#include <string>
#include "floatLog.h"
//#include "interface.h"
#include "stringStore.h"
#include <map>
#include "in.h"
#include "pthread.h"
#include "jansson.h"
#include "tinyexpr.h"

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
	friend void out_cb_in_changed(void*);
	friend void out_cb_in_invalid(void*);
	friend void out_cb_in_valid(void*);
	private:
		void *_interface;	// this is an interface*, that has the setOut(out*, float) function. Cannot mutually include.
		bool _man, _control;			// man: in manual mode. manOut wordt op de uitgang gezet. control: of uberhaupt op de uitgang gezet wordt. ~true na 1 keer setOut.
		float _min, _step, _max, _out, _manOut; // control minimum, step size and maximum. Memory for out setpoint, and manual out setpoint.
		void _setout();
		pthread_mutex_t _mutex;

		char* expression;
		int error;
		te_variable *vars = NULL;
		te_expr *expr = NULL;
		int num_vars;
		int valid_vars;		
};

extern map<string, out*> outmap;
unsigned int outsInManual();

void out_conf(json_t*);	// Modifies outs defaults by given json.
// out{
//	name:{}, name2:{}
// Supply the "out" object from the json, First members should be out's descriptors

// Callbacks: in change, in invalid and in valid.
void out_cb_in_changed(void*);
void out_cb_in_invalid(void*);
void out_cb_in_valid(void*);


#endif // HAVE_OUT_H
