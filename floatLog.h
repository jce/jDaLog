#ifndef HAVE_FLOATLOG_H
#define HAVE_FLOATLOG_H

#include <list>
#include <map>
#include "pthread.h"
#include "stdio.h"
#include <string>
#include <vector>

struct record 
{
	double t;
	float v;
} __attribute__((__packed__));

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

		void append(double, float);
		void append(record &);

		void read(std::list<record> &, double, double);
		void readBinary(std::map<double, float> &);

		void writeToFile();
		void importFromTextFile(std::string);
		void importFromBinFile(std::string); // JCE, 19-7-13
		float getLastValue();
		double getLastTime(); 	// JCE, 4-7-13
		bool get_value_at(double, float&, double&);	// When, returned value, returned time. JCE, 19-6-2019
		void readFromTo(std::map<double, float> &, double, double);	// JCE, 5-7-13
		void summaryFromTo(std::vector<flStat> &, unsigned, double, double);	// JCE, 30-12-13
		size_t getNumRecords();
		void getRecords(std::map<double, float> &m, size_t s, size_t l);

	private:
		void addDataToFloatLog(std::map<double, float> &);
		std::list<record> recordsToFile;
		std::string pathAndName;
		pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;
		pthread_mutex_t memMutex = PTHREAD_MUTEX_INITIALIZER;
		size_t records_in_file(); // Not protected by a mutex!
};

#endif // HAVE_FLOATLOG_H
