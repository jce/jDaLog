#include "equation.h"

#include "string.h"

equation::equation()
{
}

equation::~equation()
{
	for (int i = 0; i < nr_vars; i++)
		free ((vars+i)->var);
	free(vars);
	free(expression);
	te_free(expr);
	cb_free(&on_update);
	cb_free(&on_change);
	cb_free(&on_turn_invalid);
	cb_free(&on_turn_valid);
}

const char* equation::get_expression()
{
	return expression;
}

int equation::get_nr_vars()
{
	return nr_vars;
}

bool equation::get_have_default_value()
{
	return have_default_value;
}

double equation::get_default_value()
{
	return default_value;
}

const var_in_double* equation::get_vars()
{
	return vars;
}

bool equation::get_is_valid()
{
	return nr_ins == nr_ins_valid;
}

double_bool equation::get_value()
{
	double_bool rv;
	rv.value = result;
	rv.valid = get_is_valid();
	return rv;
}

void equation::get_summary(vector<flStat> *stats, unsigned int len, double start, double stop)
{
	// This can be done using the existing equation object, or by creating a new object.
	// Lets reuse the existing.

	// Prepare input data arrays
	double* indata = (double*) malloc(sizeof(double) * len * nr_vars);
	vector<flStat> summary(len);
	if (! indata)
		return;
	for (int var = 0; var < nr_vars; var++)
	{
		if (vars[var].i)
		{
			vars[var].i->getDataSummary( summary, len, start, stop);
			for (unsigned int i = 0; i < len; i++)
				indata[len*var + i] = summary[i].avg;
		}
		else
			for (unsigned int i = 0; i < len; i++)
				indata[len*var + i] = vars[var].d;
	}

	// Lock the expression, evaluate for given input data
	pthread_mutex_lock(&mutex);
	for (unsigned int i = 0; i < len; i++)
	{
		for (int var = 0; var < nr_vars; var++)
			vars[var].d = indata[len * var + i];
		(*stats)[i].avg = te_eval(expr);
	}
	pthread_mutex_unlock(&mutex);
}

void equation::eval()
{
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < nr_vars; i++)
		if (vars[i].i)
		{
			valid_ins += vars[i].i->isValid();
			vars[i].d = vars[i].i->getValue();
		}
	result = te_eval(expr);
	pthread_mutex_unlock(&mutex);
}

void equation::in_updates()
{
	eval();	// An source mighe emit an on_update before on_change. A receiver can already use the value on on_update.
	cb_call(on_update);
}

void equation::in_changes()
{
	double old_result = result;
	eval();
	if (result != old_result)
		cb_call(on_change);
}

void equation::in_turns_invalid()
{
	int old_nr_ins_valid = nr_ins_valid;
	nr_ins_valid--;
	if (old_nr_ins_valid == nr_ins)
		cb_call(on_turn_invalid);
}

void equation::in_turns_valid()
{
	nr_ins_valid++;
	if (nr_ins_valid == nr_ins)
		cb_call(on_turn_valid);
}

void equation::register_callback_on_update(void (*f)(void*), void *p)			{cb_add(&on_update, f, p);}
void equation::register_callback_on_change(void (*f)(void*), void *p)			{cb_add(&on_change, f, p);}
void equation::register_callback_on_turn_invalid(void (*f)(void*), void *p)		{cb_add(&on_turn_invalid, f, p);}
void equation::register_callback_on_turn_valid(void (*f)(void*), void *p)		{cb_add(&on_turn_valid, f, p);}

// Equator factory
//
// The json has to follow this structure:
// {
//		"expr":"Expression written in a string. For example name1 + name2 * name3",
//		"default":double, only supply if a default is desired, if omitted, 0.0 is assumed
//		"var":
//		{
//			"name1":integer,
//			"name2":double,
//			"name3":"in descriptor"
//		}
// }

equation* equation_from_json(json_t *json)
{
	equation *eq = new equation();
    te_variable *te_vars = NULL;
	bool ok = true;
	eq->expression = strdup(json_string_value(json_object_get(json, "expr")));
	json_t *default_j = json_object_get(json, "default");
	eq->have_default_value = json_number_value(default_j);
	if (eq->have_default_value)
		eq->default_value = json_number_value(default_j);

    json_t *vars_j = json_object_get(json, "var");
    const char* var_descr;
    json_t *var_j;
    if (json_is_object(vars_j))
    {
        eq->nr_vars = json_object_size(vars_j);
        int i = 0;
        eq->vars = (var_in_double*) malloc(sizeof(var_in_double) * eq->nr_vars);
		if (!eq->vars)
		{
			delete (eq);
			return NULL;
		}
	
    	te_vars = (te_variable*) malloc(sizeof(te_variable) * eq->nr_vars);
		if (!te_vars)
		{
			delete (eq);
			return NULL;
		}

        // Iterates over vars
        json_object_foreach(vars_j, var_descr, var_j)
        {
            if (json_is_string(var_j))
            {
                const char* in_descr = json_string_value(var_j);
                if (! in_descr)
                {
                    printf("Cannot read in descriptor for variable \"%s\"", var_descr);
                    ok = false;
                }
                in *in_p = get_in(in_descr);
                if (!in_p)
                {
                    printf("Cannot find in \"%s\" for variable \"%s\".\n", in_descr, var_descr);
                    ok = false;
                }
                eq->vars[i].var = strdup(var_descr);
                eq->vars[i].i = in_p;
                eq->vars[i].d = 0.0;
            }
            else if (json_is_number(var_j))
            {
                eq->vars[i].var = strdup(var_descr);
                eq->vars[i].i = NULL;
                eq->vars[i].d = json_number_value(var_j);
            }
            else
            {
                printf("Cannnot interpret content for variable \"%s\".\n", var_descr);
                ok = false;
            }
            i++;
        }

	    // tinyexpr compiles an expression. this is a translation of strings in the equation to double pointers.
	    // The double pointers should ofc. point to doubles. These can therefore not move anymore.
	    // Build the vars locations
	
	    // vars_ins are oredered identically as vars_from_conf.
	    // build the vars array itself.
	    memset(te_vars, 0, sizeof(te_variable) * eq->nr_vars);

	    for (int i = 0; i < eq->nr_vars; i++)
    	{
            te_vars[i].address = &eq->vars[i].d;
            te_vars[i].name = eq->vars[i].var;
        }

	}

    // Try and compile the expression
    int err;
    te_expr *expr = te_compile(eq->expression, te_vars, eq->nr_vars, &err);
	free (te_vars);

    if (!expr)
    {
		ok = false;
        printf("Compiling expression failed.\n\t%s\n\t%*s^\n\tError near here.\n", eq->expression, err-1, " ");
    }


	if (!ok)
	{
		delete eq;
		eq = NULL;
	}
	return eq;
}

