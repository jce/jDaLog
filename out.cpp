#include "stdio.h"
#include <string>
#include "floatLog.h"
#include "in.h"
#include "out.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "mytime.h"	// now()
#include "stringStore.h"
#include "interface.h"
#include <map>
#include "pthread.h"
#include "string.h"

using namespace std;

#define debug

// Class out. The out is an in, that can be setted with setOut. Doing this should result in immediate setting of the output in the field. The onely thing in the software knowing how to set the out is its interface, thus out should call its interface to do the work.

map<string, out*> outmap;

out::out(const string d, const string n, const string u, const unsigned int de, void* interface, float min, float step, float max) : in(d, n, u, de), _interface(interface), _man(false), _control(false), _min(min), _step(step), _max(max), _out(0), _manOut(0) 
{
	pthread_mutex_init(&_mutex, NULL);
	outmap[getDescriptor()] = this ;
}

out::~out()
{
	pthread_mutex_destroy(&_mutex);
	if(vars)
		free(vars);
	if(expr)
		te_free(expr);
}

void out::_setout()
{
	//setValue(getValue()); // outputs are discrete. Untill now the value is known and now it will be changed. JCE, 25-7-13
	// should be printed in a custom graph? stepping graph, or maybe a singleline graph and still use this line? JCE, 25-7-13
	float o = _out;
	if (_man)
		o = _manOut;
	pthread_mutex_unlock(&_mutex);
	((interface*)_interface)->setOut(this, o);
}

void out::setOut(float o)
{
	pthread_mutex_lock(&_mutex);
	if (o < _min) o = _min;
	if (o > _max) o = _max;
	_out = o;
	_control = true;
	_setout();
}

// Should be used to change the manual mode setpoint
void out::setManOut(float o)
{
	pthread_mutex_lock(&_mutex);
	if (o < _min) o = _min;
	if (o > _max) o = _max;
	_manOut = o;
	pthread_mutex_unlock(&_mutex);
}

// Should be used to get in and out of manual mode
void out::setMan(bool m)
{
	pthread_mutex_lock(&_mutex);
	_man = m;
	_setout();
}

float out::getManOut(){return _manOut;}
float out::getOut(){return _out;}
bool out::getMan(){return _man;}
bool out::getControl(){return _control or _man;}
float out::getMin(){return _min;}
float out::getStep(){return _step;}
float out::getMax(){return _max;}

void out::eval_expr()
{
	pthread_mutex_lock(&_mutex);
	if (nr_ins != valid_ins)
	{
		_control = false;
		pthread_mutex_unlock(&_mutex);
		return;
	}
	for (int i = 0; i < nr_vars; i++)
		if (vars[i].i)
			vars[i].d = vars[i].i->getValue();
	_out = te_eval(expr);
	_control = true;
	_setout();
}

unsigned int outsInManual(){
	unsigned int rv(0);
	for(map<string, out*>::iterator i = outmap.begin(); i != outmap.end(); i++)
		if (i->second->getMan())
			rv++;
	return rv;
	}

out* get_out(const char* descr)
{
	if (NULL == descr)
		return NULL;
	return get_out(string(descr));
}

out* get_out(const std::string &descr)
{
	if (outmap.count(descr))
		return outmap[descr];
	return NULL;
}

void out::getDataSummary(vector<flStat> &stats, unsigned length , double from, double to)
{
	_logger->summaryFromToWeighedN(stats, length, from, to, valid_time);
}

void out_conf(json_t* json)	// Modifies existing outs by given json.
{
	// Root level objects will be out desriptors
	const char *out_descr;
	json_t *j_out;
	json_object_foreach(json, out_descr, j_out)
	{
		//printf("%s\n", out_descr);
		out* o = get_out(out_descr);
		if (!o)
		{
			printf("Configuring out \"%s\": Out with this descriptor does not exist.\n", out_descr);
			continue;
		}
		const char *s_expr = 	json_string_value(json_object_get(j_out, "expr"));
		json_t *j_default = json_object_get(j_out, "default");
		bool have_default_val = json_is_number(j_default);
		float default_val = json_number_value(j_default);
		json_t *vars_j = json_object_get(j_out, "var");
		const char* var_descr;
		json_t *var_j;
		if (json_is_object(vars_j))
		{
			int nr_vars = json_object_size(vars_j);
			int i = 0;
			var_in_double *vars = (var_in_double*) malloc(sizeof(var_in_double) * nr_vars);
			if (!vars)
				continue;
			bool ok = true;
			// Iterates over vars
			json_object_foreach(vars_j, var_descr, var_j)
			{
				if (json_is_string(var_j))
				{
					const char* in_descr = json_string_value(var_j);
					if (! in_descr)
					{
						printf("Configuring out \"%s\": Cannot read in descriptor for variable \"%s\"", out_descr, var_descr);
						ok = false;
					}
					in *in_p = get_in(in_descr);
					if (!in_p)
					{
						printf("Configuring out \"%s\": Cannot find in \"%s\" for variable \"%s\".\n", out_descr, in_descr, var_descr);
						ok = false;
					}
					strncpy(vars[i].var, var_descr, VAR_LEN);
					vars[i].i = in_p;
					vars[i].d = 0;
				}
				else if (json_is_number(var_j))
				{
					strncpy(vars[i].var, var_descr, VAR_LEN);
					vars[i].i = NULL;
					vars[i].d = json_number_value(var_j);
				}
				else
				{
					printf("Configuring out \"%s\": Cannnot interpret content for variable \"%s\".\n", out_descr, var_descr);
					ok = false;
				}
				i++;
			}
			if (!ok)
			{
				free(vars);
				continue;
			}
	
			// tinyexpr compiles an expression. this is a translation of strings in the equation to double pointers.
			// The double pointers should ofc. point to doubles. These can therefore not move anymore.
			// Build the vars locations
			
			// vars_ins are oredered identically as vars_from_conf.
			// build the vars array itself.
			te_variable *te_vars = (te_variable*) malloc(sizeof(te_variable) * nr_vars);
			if (!te_vars)
			{
				free(vars);
				continue;
			}
			memset(te_vars, 0, sizeof(te_variable) * nr_vars);

			for (int i = 0; i < nr_vars; i++)
			{
				te_vars[i].address = &vars[i].d;				
				te_vars[i].name = vars[i].var;
			}
			
			// Try and compile the expression
			int err;
			te_expr *expr = te_compile(s_expr, te_vars, nr_vars, &err);

			if (!expr)
			{
				printf("Configuring out \"%s\": Compiling expression failed.\n\t%s\n\t%*s^\n\tError near here.\n", out_descr, s_expr, err-1, " ");
				free(te_vars);
				free(vars);
				continue;
			}

			// Program the out.
			o->expression = s_expr;
			o->vars = vars;
			free(te_vars);
			o->expr = expr;
			o->nr_vars = nr_vars;
			int nr_ins = 0;
			for (int i = 0; i < nr_vars; i++)
				nr_ins += (bool) vars[i].i;
			o->nr_ins = nr_ins;
			o->valid_ins = 0;
			for (int i = 0; i < nr_vars; i++)
				if(vars[i].i)
					o->valid_ins += vars[i].i->isValid(); 
			o->have_default_val = have_default_val;
			o->default_val = default_val;

			// Register callbacks on the in's to this out.
			for (int i = 0; i < nr_vars; i++)
				if(vars[i].i)
				{
					vars[i].i->register_callback_on_update(			out_cb_in_updated, (void*) o);
					vars[i].i->register_callback_on_change(			out_cb_in_changed, (void*) o);
        			vars[i].i->register_callback_on_turn_invalid(	out_cb_in_invalid, (void*) o);
        			vars[i].i->register_callback_on_turn_valid(		out_cb_in_valid, (void*) o);
				}
			o->eval_expr();
		}
	}
}

// Callbacks on in events
void out_cb_in_updated(void *p)
{
	out *o = (out*) p;
	o->_setout();
}

void out_cb_in_changed(void *p)
{
	out *o = (out*) p;
	o->eval_expr();
}

void out_cb_in_invalid(void *p)
{
	out *o = (out*) p;
	pthread_mutex_lock(&o->_mutex);
	o->valid_ins --;
	pthread_mutex_unlock(&o->_mutex);
	o->eval_expr();
}
void out_cb_in_valid(void *p)
{
	out *o = (out*) p;
	pthread_mutex_lock(&o->_mutex);
	o->valid_ins ++;
	pthread_mutex_unlock(&o->_mutex);
	o->eval_expr();
}

