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

using namespace std;

#define debug

// Class out. The out is an in, that can be setted with setOut. Doing this should result in immediate setting of the output in the field. The onely thing in the software knowing how to set the out is its interface, thus out should call its interface to do the work.

map<string, out*> outmap;

out::out(const string d, const string n, const string u, const unsigned int de, void* interface, float min, float step, float max) : in(d, n, u, de), _interface(interface), _man(false), _control(false), _min(min), _step(step), _max(max), _out(0), _manOut(0) {
	pthread_mutex_init(&_mutex, NULL);
	outmap[getDescriptor()] = this ;}

out::~out(){
	pthread_mutex_destroy(&_mutex);
	free(expression);
	free(vars);
	te_free(expr);
	}

void out::_setout(){
	//setValue(getValue()); // outputs are discrete. Untill now the value is known and now it will be changed. JCE, 25-7-13
	// should be printed in a custom graph? stepping graph, or maybe a singleline graph and still use this line? JCE, 25-7-13
	float o = _out;
	if (_man)
		o = _manOut;
	pthread_mutex_unlock(&_mutex);
	((interface*)_interface)->setOut(this, o);
	}

void out::setOut(float o){
	pthread_mutex_lock(&_mutex);
	if (o < _min) o = _min;
	if (o > _max) o = _max;
	_out = o;
	_control = true;
	_setout();
	//if (_man)	// Automatic is overruled by manual.
	//	((interface*)_interface)->setOut(this, _manOut);
	//else
	//	((interface*)_interface)->setOut(this, _out);
	//((interface*)_interface)->getIns();
	}

// Should be used to change the manual mode setpoint
void out::setManOut(float o){
	pthread_mutex_lock(&_mutex);
	if (o < _min) o = _min;
	if (o > _max) o = _max;
	_manOut = o;
	_setout();
	//((interface*)_interface)->setOut(this, _manOut);
	//((interface*)_interface)->getIns();
	}

// Should be used to get in and out of manual mode
void out::setMan(bool m){
	pthread_mutex_lock(&_mutex);
	_man = m;
	_setout();
	//((interface*)_interface)->setOut(this, _manOut);
	//((interface*)_interface)->getIns();
	}

float out::getManOut(){return _manOut;}
float out::getOut(){return _out;}
bool out::getMan(){return _man;}
bool out::getControl(){return _control or _man;}
float out::getMin(){return _min;}
float out::getStep(){return _step;}
float out::getMax(){return _max;}

unsigned int outsInManual(){
	unsigned int rv(0);
	for(map<string, out*>::iterator i = outmap.begin(); i != outmap.end(); i++)
		if (i->second->getMan())
			rv++;
	return rv;
	}



void out_conf(json_t*){}	// Modifies outs defaults by given json.
// out{
//	name:{}, name2:{}
// Supply the "out" object from the json, First members should be out's descriptors

// Callbacks on in events
void out_cb_in_changed(void *p)
{
	out *o = (out*) p;
}

void out_cb_in_invalid(void *p)
{
	out *o = (out*) p;
	pthread_mutex_lock(&o->_mutex);
	o->valid_vars --;
	pthread_mutex_unlock(&o->_mutex);
}
void out_cb_in_valid(void *p)
{
	out *o = (out*) p;
	pthread_mutex_lock(&o->_mutex);
	o->valid_vars ++;
	pthread_mutex_unlock(&o->_mutex);
}
