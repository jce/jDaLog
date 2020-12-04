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

#define VAR_LEN 32
typedef struct var_in_double{
	char var[VAR_LEN];
	in *i;
	double d;
} var_in_double;

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
		void eval_expr();	// Evaluate the expression.
	friend void out_cb_in_changed(void*);
	friend void out_cb_in_invalid(void*);
	friend void out_cb_in_valid(void*);
	friend void out_conf(json_t*);

		std::string expression;			// for reference
		var_in_double *vars = NULL;		// Source data for the vars table
		te_expr *expr = NULL;			// Tinyexpr equation
		int nr_vars = 0;				// Number of variables in vars_ins array.
		int nr_ins = 0;					// Number of ins the equation depends on.
		int valid_ins = 0;				// Valid source ins. Together with num_ins can disable evaluation of the equation.
		bool have_default_val = false;	// If not all ins are valid, do we set a default output?
		float default_val = 0;			// The default value if not all ins are valid.

	private:
		void *_interface;	// this is an interface*, that has the setOut(out*, float) function. Cannot mutually include.
		bool _man, _control;			// man: in manual mode. manOut wordt op de uitgang gezet. control: of uberhaupt op de uitgang gezet wordt. ~true na 1 keer setOut.
		float _min, _step, _max, _out, _manOut; // control minimum, step size and maximum. Memory for out setpoint, and manual out setpoint.
		void _setout();
		pthread_mutex_t _mutex;

};

extern map<string, out*> outmap;
unsigned int outsInManual();
out* get_out(const char*);
out* get_out(const std::string&);

void out_conf(json_t*);	// Modifies outs defaults by given json.
// out{
//	name:{}, name2:{}
// Supply the "out" object from the json, First members should be out's descriptors

// Callbacks: in change, in invalid and in valid.
void out_cb_in_changed(void*);
void out_cb_in_invalid(void*);
void out_cb_in_valid(void*);


#endif // HAVE_OUT_H
