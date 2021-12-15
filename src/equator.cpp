#include "equator.h"

#include "string.h"

equator::equator()
{
}

equator::~equator()
{
	for (int i = 0; i < nr_vars; i++)
		free ((vars+i)->var);
	free(vars);
	free(expression);
	te_free(expr);
	//cb_free(&on_eval);
	//cb_free(&on_invalid);
	//cb_free(&on_valid);
}

double_bool equator::eval()
{
	double_bool rv;
	int valid_ins;

	pthread_mutex_lock(&mutex);
	for (int i = 0; i < nr_vars; i++)
		if (vars[i].i)
		{
			valid_ins += vars[i].i->isValid();
			vars[i].d = vars[i].i->getValue();
		}
	rv.value = te_eval(expr);
	rv.valid = nr_ins == valid_ins;
	pthread_mutex_unlock(&mutex);

	return rv;
}

double* equator::get_var_address(const char *var)
{
	for (int i = 0; i < nr_vars; i++)
		if (strcmp((vars+i)->var, var) == 0)
			return & (vars+i)->d;
	return NULL;
}


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

equator* equator_from_json(json_t *json)
{
	equator *eq = new equator();
    te_variable *te_vars = NULL;
	bool ok = true;
	eq->expression = strdup(json_string_value(json_object_get(json, "expr")));
	json_t *default_j = json_object_get(json, "default");
	eq->have_default_val = json_number_value(default_j);
	if (eq->have_default_val)
		eq->default_val = json_number_value(default_j);

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

