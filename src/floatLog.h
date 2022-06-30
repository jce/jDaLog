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
		//recordlist* get_recordlist(double from, double to);	// Return a recordlist for given interval.

		// Read all
		void readBinary(std::map<double, float> &);

		// Summary read series
		// return vector, number of bins, from time, to time (break length)
		void summaryFromTo(			std::vector<flStat> &, unsigned, double, double); // Fills nr, min, avg, max based on equal sample weight.
		//void summaryFromToWeighedC(	std::vector<flStat> &, unsigned, double, double, float); // Based on sample time. Closest time weights sample.
		void summaryFromToWeighedN(	std::vector<flStat> &, unsigned, double, double, float); // Based on sample time. To next weights sample.

		// Operation mode control
		typedef enum operationmode{ to_file, ram_only } operationmode;
		// Set operation mode.
		// to_file: Default. Stores to file every "writeToFile()" call.
		// ram_only: Keep all samples in ram. "writeToFile()" has no effect.
		operationmode get_operation_mode();
		void set_operation_mode(operationmode);
		void set_ram_max_samples(size_t);	// Maximum amount of samples to be stored in ram. 0 to disable. Default is 0.
		void set_ram_max_history(float);	// Maximum historic time to be stored in ram. (Drop samples older than now-interval). 0 to disable. Default is 0.

		// Maintenance
		void writeToFile();
		void importFromTextFile(std::string);
		void importFromBinFile(std::string);
		size_t getNumRecords();

		// Trim functions remove time and/or value ranges. Including the exact matches. Return number of removed records.
		size_t remove_time_to(double);
		size_t remove_time_from(double);
		size_t remove_time_from_to(double, double);
		size_t remove_value_to(float);
		size_t remove_value_from(float);
		size_t remove_value_from_to(float, float);

		bool file_is_ok();		// Returns true if every sample in the file is later than the previous sample, none at t <= 0 and none in the future.
		size_t sort_file();		// Sorts the file. Reads all to memory, so can consume much memory.

	private:
		void addDataToFloatLog(std::map<double, float> &);
		std::list<record> recordsToFile;
		std::string pathAndName;
		pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;
		pthread_mutex_t memMutex = PTHREAD_MUTEX_INITIALIZER;
		size_t records_in_file(); // Not protected by a mutex!
		void read_last_from_file();
		record last = {0.0, 0.0};

		FILE *fp;
		void openfile_read();
		void openfile_write();
		size_t from_index(double); // File should be open, returns the first record number in the file at or above given timestamp.
		operationmode mode = to_file;
		size_t ram_max_samples = 0;
		float ram_max_history = 0;
};

#endif // HAVE_FLOATLOG_H
