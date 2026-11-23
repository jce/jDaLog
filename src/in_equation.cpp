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
#include <deque>
#include <climits>

using namespace std;

//#define DBG(...) {printf(__VA_ARGS__);printf("\n");}
#define DBG(...)

const unsigned int BLKSIZE = 262144;

// In version that derives its value from an equation. Equations use only constants and ins.

in_equation::in_equation(string d, string n, string u, const unsigned int de) : in(d, n, u, de) 
{
    ineqmap_mutex.lock();
    in_eq_map[d] = this;
    ineqmap_mutex.unlock();
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

void in_equation::getDataSummary(vector<flStat> &stats, unsigned int bins, double from, double to)
{
    // As a major sin against the gods of OO, i have copied this from floatLog.cpp.
    // the in_equation is supposed to not have a floatlog and still needs a summary so
    // the logic is copied here. JCE, 20-11-2026

    DBG("%s getDataSummary(stats, %u, %f, %f)", _descr.c_str(), bins, from, to);
    int b;
	double interval =(( double) to - from) / bins;
    double t, t1, dur, tcur, tend, binedge, dt;
    float v = NAN, v1 = NAN, v2, slope, va, vb, lo, hi;
    bool havetv1 = false, havev2 = false, lastrun = false, more = true;
    deque<record> records;
    t = from;

    while (true)
    {
        if (lastrun)
            break;
        if (records.size() == 0)
        {
            if (more)
            {
                more = getDataOrNaN(records, t, to, BLKSIZE);
                for (unsigned i = 0; i < records.size(); i++)
                    DBG("Rec %d: t=%f, v=%f", i, records[i].t, records[i].v);
            }     
            else // 0 records, not more
                lastrun = true;
        }
        DBG("records.size() = %lu, more = %d", records.size(), more);
        if (lastrun)
        {
            t = to;
            v = v1;
        }
        else // Not lastrun
        {
            if (records.size())
            {
                t = records[0].t;
                v = records[0].v;
                records.pop_front();
            }
        }
 
        if (havetv1) // Only upon reading sample x, do we know the duration for sample x-1.
        {
            dur = t-t1;                  // Duration that v1 is valid.
            tcur = fmax(t1, from);       // Start from within the binrange.
            tend = fmin(t1+dur, to);     // End at most at the last bin edge.
            slope = (v - v1) / (t - t1); // Slope of the line

            while (tcur < tend)          // Deposit in all intermediate bins.
            {
                b = (int) (( tcur - from ) / interval );
                if (from + interval * (b+1) <= tcur)
                    b++;                    // Prevents getting locked on the edge.
                binedge = from + interval * ( b + 1);
                dt = fmin(tend, binedge) - tcur;
                va = v1 + slope * (tcur - t1);
                vb = v1 + slope * (fmin(tend, binedge) - t1);
                lo = fmin(va, vb);
                hi = fmax(va, vb);
    
                if (stats[b].t == 0.0)
                {
                    stats[b].min = lo;
                    stats[b].max = hi;
                }
                else
                {
                    stats[b].min = fmin(stats[b].min, lo);
                    stats[b].max = fmax(stats[b].max, hi);
                }
                
                if (!isnan(va) && !isnan(vb))
                {
                    stats[b].avg += 0.5 * (va+vb) *dt;
                    stats[b].t += dt;
                }
                tcur += dt;
            }

            if (t1 >= from && t1 < to)
            {
                b = (int) ((t1 - from) / interval);
                stats[b].nr++;      // Count the amount of samples
                if (havev2)
                {   
                    stats[b].pedge += v1 > v2;
                    stats[b].nedge += v1 < v2;
                }
            }
            v2 = v1;
            havev2 = true;
        }
        v1 = v;
        t1 = t;
        havetv1 = true;
    }

	// Calculate the average
	for(unsigned i = 0; i<bins; i++)
		if (stats[i].t)
        {
			stats[i].avg = stats[i].avg / stats[i].t;
            DBG("bin: %i, samples: %i, min: %f, avg: %f, max: %f", i, stats[i].nr, stats[i].min, stats[i].avg, stats[i].max);
        }
}

bool in_equation::getDataOrNaN(deque<record> &records, double from, double to, unsigned num)
{
    DBG("%s getDataOrNaN(records, %f, %f, %u)", _descr.c_str(), from, to, num);
    // vector<eqVarDescr> vardes = {descr, i, idx}
    int nrIns = vardes.size();              // Number of ins
    vector<deque<record>> cache(nrIns);     // Intermediate cache
    vector<bool> more(nrIns, true);         // Is there more for these ins?
    vector<float> var(nrIns, NAN);          // Parameter for eval()
    unsigned count = 0;

    // Zip loop
    double t = from;
    double lastEmit = -1e300;
    while (t < to and count < num)
    {
        DBG("t = %f, count = %u", t, count);
        // First, get new data if cache is low
        for (int i = 0; i < nrIns; i++)
        {
            
            if (cache[i].size() == 0 and more[i])
            {
                more[i] = vardes[i].i->getDataOrNaN(cache[i], t, to, BLKSIZE);
                for (unsigned j = 0; j < cache[i].size(); j++)
                    DBG("Rec %d: t=%f, v=%f", j, cache[i][j].t, cache[i][j].v);
            }
            DBG("%s cache[%u].size() = %lu more = %i", _descr.c_str(), i, cache[i].size(), (int) more[i]);
        }
        
        // Find the time of the first next change. Can be t.
        double tnext = to;
        bool any = false;
        for (int i = 0; i < nrIns; i++)
            if (cache[i].size() > 0)
            {
                any = true;
                if (cache[i][0].t < tnext)
                    tnext = cache[i][0].t;
            }
        if (!any) return false;

        // Fill var, only the changed
        for (int i = 0; i < nrIns; i++)
            while (cache[i].size() > 0 and cache[i][0].t == tnext)
            {
                var[i] = cache[i][0].v;
                cache[i].pop_front();
            }

        // Dont emit if this is not a later value
        if (tnext < from or tnext <= lastEmit)
            continue;

        // Calculate the equation for this inputs
        records.push_back({tnext, eq->eval(var)}); 
        count++;
        lastEmit = tnext;
        t = tnext;
    }
    return true;
}

void in_equation::getData(map<double, float>& data, double from, double to)
{
    deque<record> records;
    getDataOrNaN(records, from, to, INT_MAX);
    for (auto r : records)
        if (!isnan(r.v))
            data[r.t] = r.v;
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
    //DBG("%s.updateValue() called.", _descr.c_str());
    vector<float> var;
    for (auto v: vardes)
    {
        //DBG("%s: %p", v.descr.c_str(), v.i);
        if (v.i->isValid())
            var.push_back( v.i->getValue() );
        else
            var.push_back( NAN );
    }
    float prev = _value;
    _value = eq->eval(var);
    _isValid = not isnan(_value);
    _time = now();
    //DBG("%s: val %f, time %f", _descr.c_str(), _value, _time);

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

const vector<eqVarDescr> in_equation::getVardes() { return vardes; }
const eqNode* in_equation::getEq() { return eq; }

mutex ineqmap_mutex;
map<string, in_equation*> in_eq_map;

in_equation* getInEq(string name)
{   
    in_equation* rv = 0;
    ineqmap_mutex.lock();
    if(in_eq_map.count(name) == 1)
        rv = in_eq_map[name];
    ineqmap_mutex.unlock();
    return rv;
}


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

