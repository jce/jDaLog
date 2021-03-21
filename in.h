#ifndef HAVE_IN_H
#define HAVE_IN_H

#include <map>
#include <mutex>
#include "stdio.h"
#include <string>

#include "callback.h"
#include "floatLog.h"
#include "job_sched.h"
#include "main.h"
#include "stringStore.h"

// The In has a separate name and descriptor. The second is used for file identification, so a namechange wont result in losing all measurement data. JCE, 20-6-13

class in{
	public:
		//in(const string); // descr
		in(const std::string, const std::string name = "", const std::string units = "", const unsigned int decimals = 0); // descr, name, units, decimals in floating point representation.
		in(uint8_t, const char *dir, const std::string descr, const std::string prefix); // bogus item as hack, storage location, descr, prefix for name.
		virtual ~in();
		bool hidden = false;
		virtual void setValue(float, double = 0);
		void setVal(float, double = 0);
		float getValue();
		bool get_value_at(double, float&, double&);	// When, returned value, returned time. JCE, 19-6-2019
		float getVal();
		double getTime();		// JCE, 4-7-13
		double getAge();		// JCE, 4-7-13
		//void setValid(bool);
		bool isValid();
		const std::string getDescriptor();
		const std::string getUnits();	// JCE, 20-6-13
		const std::string getName();		// JCE, 20-6-13
		const std::string getNote();		// JCE, 20-6-13
		void setNote(std::string);		// JCE, 4-7-13
		unsigned int getDecimals();
		void writeToFile();		// JCE, 1-7-13
		void getData(map<double, float> &, double, double);	// JCE, 5-7-13 
		size_t getNumRecords();
		void getRecords(map<double, float> &, size_t, size_t);	// JCE, 5-7-13 
		// getDataSummary is a semi-draconic function to get a data summary (min, avg, max, samples-in-bin) without creating a list or map with every intermediate value. Should result in saving of memory and processor time. double 1 is the start-time, double 2 the end-time. 
		virtual void getDataSummary(vector<flStat>&, unsigned, double, double);
		void importData();		// JCE, 18-7-13, JCE, should kick the floatLogger into importing data from some configured filenames.
		void touch();			// JCE, 28-8-13, makes a new measurement point, equal to the previous value.
		void set_valid_time(float);	// Sets the time for what the measurement remains valid.
		float get_valid_time();	// Reads the time that a measurement remains valid.
		void sort_file();		// Sort the binary file.		


		// Callbacks on specific events. JCE, 9-11-2020
		void register_callback_on_update(void (*)(void*), void*);
		void register_callback_on_change(void (*)(void*), void*);
		void register_callback_on_turn_invalid(void (*)(void*), void*);
		void register_callback_on_turn_valid(void (*)(void*), void*);
		
	protected:
		const unsigned int _decimals;
		float _value;
		double _time;			// JCE, 4-7-13
		floatLog *_logger;
		bool _isValid, _isKnown;	// isKnown will be set if _value is input or read from file. JCE, 25-6-13
		const std::string _descr;
		stringStore *_name, *_units, *_note; // JCE, 20-6-13
		const std::string name_prefix = "";
		callback_list *on_update = NULL;
		callback_list *on_change = NULL;
		callback_list *on_turn_invalid = NULL;
		callback_list *on_turn_valid = NULL;
		float valid_time = 60;	// Default value is 60 seconds.
		void turn_invalid();
		CC(in, turn_invalid);
	};

extern mutex inmap_mutex; // JCE, 9-10-2018
extern std::map<std::string, in*> inmap;
in* get_in(const char*); // JCE, 9-12-2020
in* get_in(std::string); // JCE, 19-6-2019
#endif // HAVE_IN_H
