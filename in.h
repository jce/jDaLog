#ifndef HAVE_IN_H
#define HAVE_IN_H

#include "stdio.h"
#include <string>
#include "floatLog.h"
#include "stringStore.h"
#include <map>
//#include <vector>
#include <mutex>

using namespace std;

// The In has a separate name and descriptor. The second is used for file identification, so a namechange wont result in losing all measurement data. JCE, 20-6-13

class in{
	public:
		//in(const string); // descr
		in(const string, const string name = "", const string units = "", const unsigned int decimals = 0); // descr, name, units, decimals in floating point representation.
		in(uint8_t, const char *dir, const string descr, const string prefix); // bogus item as hack, storage location, descr, prefix for name.
		~in();
		void setValue(float, double = 0);
		void setVal(float, double = 0);
		float getValue();
		bool get_value_at(double, float&, double&);	// When, returned value, returned time. JCE, 19-6-2019
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
		size_t getNumRecords();
		void getRecords(map<double, float> &, size_t, size_t);	// JCE, 5-7-13 
		// getDataSummary is a semi-draconic function to get a data summary (min, avg, max, samples-in-bin) without creating a list or map with every intermediate value. Should result in saving of memory and processor time. double 1 is the start-time, double 2 the end-time. 
		void getDataSummary(vector<flStat>&, unsigned, double, double); // JCE, 31-12-13
		void importData();		// JCE, 18-7-13, JCE, should kick the floatLogger into importing data from some configured filenames.
		void touch();			// JCE, 28-8-13, makes a new measurement point, equal to the previous value.
	private:
		const unsigned int _decimals;
		float _value;
		double _time;			// JCE, 4-7-13
		floatLog *_logger;
		bool _isValid, _isKnown;	// isKnown will be set if _value is input or read from file. JCE, 25-6-13
		const string _descr;
		stringStore *_name, *_units, *_note; // JCE, 20-6-13
		const string name_prefix = "";
	};

extern mutex inmap_mutex; // JCE, 9-10-2018
extern map<string, in*> inmap;
in* get_in(string); // JCE, 19-6-2019
#endif // HAVE_IN_H
