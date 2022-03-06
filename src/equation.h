#ifndef HAVE_EQUATOR_H
#define HAVE_EQUATOR_H

#include "callback.h"
#include "in.h"
#include "jansson.h"
#include "pthread.h"
#include "tinyexpr.h"

// The equation is a collection of structs and functions intended to separate
// equations out of object logics in one central place.
// It works with in's but is itself not an in.

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

class equation;
equation* equation_from_json(json_t*);	// Builds an equation from json.

class equation
{
public:
	equation();
	~equation();

	const char* get_expression();
	int get_nr_vars();
	int get_nr_ins();
	bool get_have_default_value();
	double get_default_value();
	const var_in_double* get_vars();
	bool get_is_valid();
	double_bool get_value();
	void get_summary(vector<flStat>*, unsigned int len, double start, double stop); // Will only fill out the average.

	void in_updates();			CC(equation, in_updates);
	void in_changes();			CC(equation, in_changes);
	void in_turns_invalid();	CC(equation, in_turns_invalid);
	void in_turns_valid();		CC(equation, in_turns_valid);
	void register_callback_on_update(void (*)(void*), void*);
	void register_callback_on_change(void (*)(void*), void*);
	void register_callback_on_turn_invalid(void (*)(void*), void*);
	void register_callback_on_turn_valid(void (*)(void*), void*);

private:
	void eval();

	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	char *expression;         		// for reference
	var_in_double *vars = NULL;     // Source data for the vars table
	te_expr *expr = NULL;           // Tinyexpr equation
	//callback_list *on_eval, *on_invalid, *on_valid;
	int nr_vars = 0;				// Length of vars
	int nr_ins = 0;					// Number of ins in the vars
	int nr_ins_valid = 0;			// Number of ins that are valid
	int valid_ins = 0;				// Number of valid ins in the vars
	bool have_default_value = false;// Is there a default value?
	double default_value = 0.0;		// The default value (if applicable).
	double previous_result = 0.0;	// For value change detection.
	double result = 0.0;			// Current result.

	callback_list *on_update = NULL;
	callback_list *on_change = NULL;
	callback_list *on_turn_invalid = NULL;
	callback_list *on_turn_valid = NULL;

    friend equation* equation_from_json(json_t*);
};





#endif // HAVE_EQUATOR_H

