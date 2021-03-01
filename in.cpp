#include "stdio.h"
#include <string>
#include "floatLog.h"
#include "in.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "mytime.h"	// now()
#include "stringStore.h"
#include <map>

using namespace std;

//#define debug

// Implementation of class in. In should give an uniform representation of inputs for tcFarmControl. I assume to use it later for both visualisation and as generic origin of information for internal logics. The in should also allow for "simple" enumeration and finding of any of the inputs. To be done by an in-map or list with search function. JCE, 19-6-13

mutex inmap_mutex;
map<string, in*> inmap;

in::in(const string d, const string n, const string u, const unsigned int de) : _decimals(de), _value(0), _time(0), _logger(NULL), _isValid(false),_isKnown(false), _descr(d), _name(NULL), _units(NULL), _note(NULL) {
	string path = tcDataDir;	
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	path += "in/";
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	path += d;
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	//s += "/floatLog.bin";
	_logger = new floatLog(path + "/floatLog.bin");
	_units = new stringStore(path + "/units.txt");
	if (_units->getString() != u and u != "") _units->setString(u);
	_name = new stringStore(path + "/name.txt");
	if (_name->getString() != n and n != "") _name->setString(n);
	_note = new stringStore(path + "/note.txt");
	//printf("eind init floatlog\n");
	
	inmap_mutex.lock();	
	if (inmap.find(_descr) != inmap.end()) printf("Double description used for ""in"" %s\n", _descr.c_str());
	inmap[_descr] = this ;
	inmap_mutex.unlock();}	

in::in(uint8_t, const char *dir, const string d, const string prefix): _decimals(6), _value(0), _time(0), _logger(NULL), _isValid(false),_isKnown(false), _descr(d), _name(NULL), _units(NULL), _note(NULL), name_prefix(prefix)
{
	_logger = new floatLog(string(dir) + "/floatLog.bin");
	_units = new stringStore(string(dir) + "/units.txt");
	_name = new stringStore(string(dir) + "/name.txt");
	_note = new stringStore(string(dir) + "/note.txt");
	
	inmap_mutex.lock();	
	if (inmap.find(_descr) != inmap.end()) printf("Double description used for ""in"" %s\n", _descr.c_str());
	inmap[_descr] = this ;
	inmap_mutex.unlock();
}	

in::~in()
{
	cb_free(&on_update);
	cb_free(&on_change);
	cb_free(&on_turn_invalid);
	cb_free(&on_turn_valid);

	#ifdef debug
		printf("Destroying %s ...remove self from inmap: %s\nInmap contains: ", _descr.c_str(), _descr.c_str());
		fflush(stdout);
		for (map<string,in*>::iterator i=inmap.begin(); i!=inmap.end(); ++i){
			printf("%s ", i->first.c_str());
			fflush(stdout);
			}
		printf("\n");
		fflush(stdout);
	#endif
	inmap_mutex.lock();	
	inmap.erase(_descr);
	inmap_mutex.unlock();	
	#ifdef debug
		printf("Destroying %s ...destroying floatLog\n", _descr.c_str());
		fflush(stdout);
	#endif
	//_logger->~floatLog();
	delete _logger; // JCE, 30-12-13
	#ifdef debug
		printf("Destroying %s ...destroying stringStore\n", _descr.c_str());
		fflush(stdout);
	#endif
	//_units->~stringStore();
	delete _units;
	#ifdef debug
		printf("Destroying %s ...destroying another stringStore\n", _descr.c_str());
		fflush(stdout);
	#endif
	//_name->~stringStore();
	delete _name;
	#ifdef debug
		printf("Destroying %s ...destroying a third stringStore\n", _descr.c_str());
		fflush(stdout);
	#endif
	//_note->~stringStore();
	delete _note;
	#ifdef debug
		printf("In %s destroyed\n", _descr.c_str());
		fflush(stdout);
	#endif
	}

void in::setValue(float v, double t)
{
	bool changed, tvalid;
	changed = !(_value == v);
	tvalid = ! _isValid;

	_value = v;
	_isValid = true;
	_isKnown = true;
	if (t == 0) t = now();		// fixed t!= 0 -> t==0. JCE, 4-7-13
	_time = t;
	_logger->append(t, v);

	cb_call(on_update);
	if (changed)
		cb_call(on_change);
	if (tvalid)
		cb_call(on_turn_valid);
}

void in::setVal(float v, double t){
	setValue(v, t);}

float in::getValue()
{
	if (not _isKnown)
	{
		record r = _logger->getLast();
		_value = r.v;
		_time = r.t;
		_isKnown = r.t != 0;
	}
	return _value;
}

bool in::get_value_at(double time, float &value, double &value_time)	// When, returned value, returned time. JCE, 19-6-2019
{
	return _logger->get_value_at(time, value, value_time);
}

double in::getTime()
{
	if (not _isKnown)
	{
		record r = _logger->getLast();
		_value = r.v;
		_time = r.t;
		_isKnown = r.t != 0;
	}
	return _time;
}

double in::getAge(){
	return now() - getTime();}

float in::getVal(){
	return getValue();}

void in::setValid(bool v)
{
	bool tinvalid = _isValid && !v;
	bool tvalid = !_isValid && v;
	_isValid = v;
	if (tinvalid)
		cb_call(on_turn_invalid);
	if (tvalid)
		cb_call(on_turn_valid);
}

bool in::isValid(){
	return _isValid;}

const string in::getDescriptor(){
	return _descr;}

const string in::getUnits(){
	return _units->getString();}

const string in::getName(){
	return name_prefix + _name->getString();}

const string in::getNote(){
	return _note->getString();}

void in::setNote(string newNote){
	_note->setString(newNote);}

unsigned int in::getDecimals(){
	return _decimals;}

void in::writeToFile(){	// Pass to floatlog. JCE, 1-7-13
	_logger->writeToFile();}

void in::getData(map<double, float>& data, double from, double to){
	_logger->readFromTo(data, from, to);}

size_t in::getNumRecords()
{
	return _logger->getNumRecords();
}

void in::getRecords(map<double, float> &m, size_t s, size_t l)
{
	_logger->getRecords(m, s, l);
}

void in::getDataSummary(vector<flStat> &stats, unsigned length , double from, double to){ // JCE, 31-12-13
	_logger->summaryFromTo(stats, length, from, to);}

// Function to start importing a preconfigured text file. JCE, 18-7-13
void in::importData(){
	_logger->importFromTextFile((string) tcDataDir + "/in/" + _descr + "/import.txt");
	_logger->importFromBinFile((string) tcDataDir + "/in/" + _descr + "/import.bin");
	}

void in::touch()
{
	setValue(getValue());
}

in* get_in(const char *s)
{
	if (!s)
		return NULL;
	return get_in(string(s));
}

in* get_in(string name)
{
	in* rv = 0;
	inmap_mutex.lock();
	if(inmap.count(name) == 1)
		rv = inmap[name];
	inmap_mutex.unlock();
	return rv;
}

// Callbacks on specific events. JCE, 9-11-2020
void in::register_callback_on_update(void (*f)(void*), void *p)			{cb_add(&on_update, f, p);}
void in::register_callback_on_change(void (*f)(void*), void *p)			{cb_add(&on_change, f, p);}
void in::register_callback_on_turn_invalid(void (*f)(void*), void *p)	{cb_add(&on_turn_invalid, f, p);}
void in::register_callback_on_turn_valid(void (*f)(void*), void *p)		{cb_add(&on_turn_valid, f, p);}
