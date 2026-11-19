#include "in_equation.h"

#include "floatLog.h"
#include "in.h"
#include "main.h"
#include <map>
#include "stdio.h"
#include <string>
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include <cmath>

using namespace std;

//#define DBG(...) printf(__VA_ARGS__);printf("\n");
#define DBG(...)

// In version that derives its value from an equation. Equations use only constants and ins.

in_equation::in_equation(string d, string n, string u, const unsigned int de) : in(d, n, u, de) 
{
    //in_eq_list.push_back(this);
	//eq->register_callback_on_update( cc_in_equation_call_on_equation_result_update, this );
	//eq->register_callback_on_change( cc_in_equation_call_on_equation_result_change, this );
	//eq->register_callback_on_turn_invalid( cc_in_turn_invalid, this );
	//eq->register_callback_on_turn_valid( cc_in_equation_call_on_turn_valid, this );
    _isKnown = true;
}	

in_equation::~in_equation()
{
	delete eq;
}

void in_equation::getDataSummary(vector<flStat>&, unsigned int, double, double)
{
}

//void in_equation::call_on_equation_result_update(){ updateValue(); }
//void in_equation::call_on_equation_result_change(){ updateValue(); }
//void in_equation::call_on_turn_valid(){             updateValue(); }
//void in_equation::call_on_turn_invalid(){           updateValue(); }

bool valueDifferent(float a, float b)
{
    if (isnan(a) and isnan(b)) return false;
    if (isnan(a) or isnan(b)) return true;
    return a != b;
}

void in_equation::updateValue()
{
    DBG("%s.updateValue() called.", _descr.c_str());
    vector<float> var;
    for (auto v: vardes)
    {
        DBG("%s: %p", v.descr.c_str(), v.i);
        if (v.i->isValid())
            var.push_back( v.i->getValue() );
        else
            var.push_back( NAN );
    }
    float prev = _value;
    _value = eq->eval(var);
    _isValid = not isnan(_value);
    _time = now();
    DBG("%s: val %f, time %f", _descr.c_str(), _value, _time);

	cb_call(on_update);
	if (valueDifferent(prev, _value))
		cb_call(on_change);
	if (not isnan(prev) and isnan(_value))
		cb_call(on_turn_valid);
	if (isnan(prev) and not isnan(_value))
		cb_call(on_turn_invalid);
}

void in_equation::registerCallbacks()
{
    for (auto v: vardes)
    {
        v.i->register_callback_on_update(in_equation::cc_in_equation_updateValue, this);
        v.i->register_callback_on_change(in_equation::cc_in_equation_updateValue, this);
        v.i->register_callback_on_turn_invalid(in_equation::cc_in_equation_updateValue, this);
        v.i->register_callback_on_turn_valid(in_equation::cc_in_equation_updateValue, this);
    }
}

list<in_equation*> in_eq_list;

void buildInEquations(json_t *input_data)
{
	if (! json_is_object(input_data))
		return;
	json_t *json, *jdecimals;
	const char *id;
	const char *name;
	const char *unit;
	int decimals;

	json_object_foreach(input_data, id, json)
	{
		name = NULL;
		unit = NULL;
		decimals = 0;

		if (json_is_object(json))
		{
			name = 	json_string_value(json_object_get(json, "name"));
			if (!name)
				name = id;
			unit =	json_string_value(json_object_get(json, "unit"));
			if (!unit)
				unit = "";
			jdecimals = json_object_get(json, "decimals");
			if (json_is_number(jdecimals))
				decimals = json_number_value(jdecimals);
			else
				decimals = 0;

            json_t *jeq = json_object_get(json, "eq");
            if (!jeq)
            {
                printf("Could not build in_equation %s: No eq field\n", id);
                continue;
            }
            vector<eqVarDescr> vardes;
            eqNode *node = eqFromJson(jeq, vardes);
            if (node == nullptr)
            {
                printf("Could not build equation.\n");
                continue;
            }
			
			in_equation *i = new in_equation(id, name, unit, decimals);
            i->vardes = vardes;
            i->eq = node;
            i->registerCallbacks();
            DBG("Callbacks registerred");
		}
	}
}

