#ifndef HAVE_EQUATOR_H
#define HAVE_EQUATOR_H

//#include "callback.h"
#include "in.h"
#include "jansson.h"
#include "pthread.h"
#include "tinyexpr.h"

// The equator is a collection of structs and functions intended to separate
// equations out of object logics in one central place.

typedef struct double_bool
{
	double value;
	bool valid;
} double_bool;

typedef struct var_in_double
{
    char *var;
    in *i;
    double d;
} var_in_double;

class equator;
equator* equator_from_json(json_t*);	// Builds an equator from json.

class equator
{
public:
	equator();
	~equator();
	double_bool eval();
	double* get_var_address(const char*);
private:
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	char *expression;         		// for reference
	var_in_double *vars = NULL;     // Source data for the vars table
	te_expr *expr = NULL;           // Tinyexpr equation
	//callback_list *on_eval, *on_invalid, *on_valid;
	int nr_vars = 0;				// Length of vars
	int nr_ins = 0;					// Number of ins in the vars
	int valid_ins = 0;				// Number of valid ins in the vars
	bool have_default_val = false;	// Is there a default value?
	double default_val = 0.0;		// The default value (if applicable).

    friend equator* equator_from_json(json_t*);
};





#endif // HAVE_EQUATOR_H

