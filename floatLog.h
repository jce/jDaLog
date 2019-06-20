#ifndef HAVE_FLOATLOG_H
#define HAVE_FLOATLOG_H

#include "stdio.h"
#include <list>
#include <map>
#include <string>
#include <vector>
#include "pthread.h"
#include "floatLog.h"
using namespace std;

struct record {
	double t;
	float v;
	} __attribute__((__packed__));

struct flStat{
	int nr; 
	float min; 
	double avg; 
	float max;};

class floatLog{
	public:
		floatLog(string);
		~floatLog();
		void append(list<record> &);
		void append(double, float);
		void append(record &);
		void read(list<record> &, double, double);
		void readBinary(map<double, float> &);
		void writeToFile();
		void importFromTextFile(string);
		void importFromBinFile(string); // JCE, 19-7-13
		float getLastValue();
		double getLastTime(); 	// JCE, 4-7-13
		bool get_value_at(double, float&, double&);	// When, returned value, returned time. JCE, 19-6-2019
		void readFromTo(map<double, float> &, double, double);	// JCE, 5-7-13
		void summaryFromTo(vector<flStat> &, unsigned, double, double);	// JCE, 30-12-13
	private:
		void _addDataToFloatLog(map<double, float> &);
		list<record> _recordsToFile;
		string _pathAndName;
		//void writeToFile();
		//void _read(map<double, float> &);
		pthread_mutex_t _fileMutex, _memMutex;
		};

#endif // HAVE_FLOATLOG_H
