#ifndef HAVE_INOUT_H
#define HAVE_INOUT_H

#include "stdio.h"
#include <string>
#include "floatLog.h"
#include "stringStore.h"
#include <map>
#include "in.h"

using namespace std;

// Ok, the inout. This class should represent an output of the internal logic. Its instance is (usually) member of the interface or io communication object. It is an IN, for it can be considered as an in: it can be read and used as in, and should log its status just as an in. Its state should be set in logic, 
//
// is its status defined by what the logic wants it to be, or what is actually measured... Also, is its internal state updated immediately, or at the end of a classic PLC scan cycle: read, logic, set.
// the inout is active on both logic and set parts, tough the in only on read part.  

class inout:public in{
	public:
		//in(const string); // descr
		in(const string, const string name = "", const string units = "", const unsigned int decimals = 0); // descr, name, units, decimals in floating point representation.
		~in();
		void setValue(float, double = 0);
		void setVal(float, double = 0);
		float getValue();
		float getVal();
		double getTime();		// JCE, 4-7-13
		double getAge();		// JCE, 4-7-13
		void setValid(bool);
		bool isValid();
		const string getDescriptor();
		const string getUnits();	// JCE, 20-6-13
		const string getName();		// JCE, 20-6-13
		const string getNote();		// JCE, 20-6-13
		void setNote(string);		// JCE, 4-7-13
		unsigned int getDecimals();
		void writeToFile();		// JCE, 1-7-13
		void getData(map<double, float> &, double, double);	// JCE, 5-7-13 
	private:
		const unsigned int _decimals;
		float _value;
		double _time;			// JCE, 4-7-13
		floatLog *_logger;
		bool _isValid, _isKnown;	// isKnown will be set if _value is input or read from file. JCE, 25-6-13
		const string _descr;
		stringStore *_name, *_units, *_note; // JCE, 20-6-13
	};

extern map<string, out*> outmap;

#endif // HAVE_IN_H
