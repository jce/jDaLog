#ifndef HAVE_FLOATLOG_H
#define HAVE_FLOATLOG_H

#include <list>
#include <map>
#include "pthread.h"
#include "stdio.h"
#include <string>
#include <vector>

typedef struct record
{
	double t;
	float v;
} record;

struct flStat
{
	unsigned int nr;// Number of samples in interval.
	float t;		// Total described time [s]. 
	float min; 		// Smallest sample.
	double avg;		// Time weighed average. 
	float max;		// Largest sample.
};

class floatLog
{
	public:
		floatLog(std::string);
		~floatLog();

		// Data feeder
		void append(double, float);

		// Read single
		record getLast();	// Gets last from disk.
		bool get_value_at(double, float&, double&);	// When, returned value, returned time.

		// Read series
		void readFromTo(std::map<double, float> &, double, double);		 // return map, first record time, last record time
		void getRecords(std::map<double, float> &m, size_t s, size_t l); // return map, first record number, last record number

		// Read all
		void readBinary(std::map<double, float> &);

		// Fancy read series
		void summaryFromTo(std::vector<flStat> &, unsigned, double, double);

		// Maintenance
		void writeToFile();
		void importFromTextFile(std::string);
		void importFromBinFile(std::string);

		size_t getNumRecords();

	private:
		void addDataToFloatLog(std::map<double, float> &);
		std::list<record> recordsToFile;
		std::string pathAndName;
		pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;
		pthread_mutex_t memMutex = PTHREAD_MUTEX_INITIALIZER;
		size_t records_in_file(); // Not protected by a mutex!
};

#endif // HAVE_FLOATLOG_H
