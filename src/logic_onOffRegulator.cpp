#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "logic_onOffRegulator.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "out.h"
#include "unistd.h" // usleep
#include "math.h"	// pow
#include "webin.h"
#include "webgui.h"	// plotlines

using namespace std;

//#define debug


logic_onOffRegulator::logic_onOffRegulator(const string d, const string n, in *i, out *o, string dir, in *sp, in *low, in *high) : logic(d, n), _in(i), _low(low), _high(high), _sp(sp), _e(NULL), _out(o), _dir(dir), _myLow(false), _myHigh(false), _mySP(false) {
	if (not _low){
		_myLow = true;
		_low = new webin(getDescriptor() + "_low", getName() + " low limit", _in->getUnits(), _in->getDecimals());}
	if (not _high){
		_myHigh = true;
		_high = new webin(getDescriptor() + "_high", getName() + " high limit", _in->getUnits(), _in->getDecimals());}
	if (not _sp){
		_mySP = true;
		_sp = new webin(getDescriptor() + "_sp", getName() + " setpoint", _in->getUnits(), _in->getDecimals());}
	_e = new in(getDescriptor() + "_e", getName() + " error", _in->getUnits(), _in->getDecimals());
	}

logic_onOffRegulator::~logic_onOffRegulator(){
	if(_myLow)
		delete _low;
	if(_myHigh)
		delete _high;
	if(_mySP)
		delete _sp;
	delete _e;
	}

void logic_onOffRegulator::run(){
	// The on-off switch thing
	double out = _out->getValue();
	double sp = _sp->getValue();
	double error = _in->getValue() - sp;
	_e->setValue(error);
	if (_dir == "-"){
		if (_in->getValue() > sp+_high->getValue())
			out = 1;
		if (_in->getValue() < sp+_low->getValue())
			out = 0;
		}
	else{ // _dir is most likely "+", but catch all 
		if (_in->getValue() > sp+_high->getValue())
			out = 0;
		if (_in->getValue() < sp+_low->getValue())
			out = 1;
		}
	_out->setOut(out);
	}

void logic_onOffRegulator::setOut(out*, float){}

int logic_onOffRegulator::make_page(struct mg_connection *conn){

	mg_printf(conn, "on/off regulator page. This page displays the current state as well as configured values for this on-off regulator<br>\n");
	mg_printf(conn, "Lower treshold: %.*f %s<br>\n", _low->getDecimals(), _low->getValue(), _low->getUnits().c_str());
	mg_printf(conn, "Higher treshold: %.*f %s<br>\n", _high->getDecimals(), _high->getValue(), _high->getUnits().c_str());
	mg_printf(conn, "Error value: %.*f %s<br>\n", _e->getDecimals(), _e->getValue(), _e->getUnits().c_str());
	mg_printf(conn, "Output value: %.*f %s<br>\n", _out->getDecimals(), _out->getValue(), _out->getUnits().c_str());

	string line;
 	line = make_image_line(plotLines(_low, _high, _e, _out, now() - 3600, now(), 1280, 300, "low, high, error and output"));
	mg_printf(conn, line.c_str());
 	//line = make_image_line(plotLines(_low, _high, _e, _out, now() - 24*3600, now(), 1280, 300, "low, high, error and output"));
	//mg_printf(conn, line.c_str());
	return 1;
	}

