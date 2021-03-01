#include "floatLog.h"

#include <cmath>
#include <fstream>
#include <list>
#include <map>
#include "pthread.h"
#include <sstream>
#include "stdio.h"
#include <string>

using namespace std;

#define FOR_ALL_IN_FILE_UNM(...) \
{ \
	FILE *fp; \
	double t; \
	float v; \
    fp = fopen(pathAndName.c_str(), "r"); \
    if (fp) \
    { \
        while ( fread(&t, sizeof(t), 1, fp) && fread(&v, sizeof(v), 1, fp) ) \
		{ \
			__VA_ARGS__ \
        } \
        fclose(fp); \
    } \
}

#define FOR_ALL_IN_FILE(...) \
	pthread_mutex_lock(&fileMutex); \
	FOR_ALL_IN_FILE_UNM(__VA_ARGS__) \
    pthread_mutex_unlock(&fileMutex); \

#define FOR_ALL_IN_MEM(...) \
{ \
	double t; \
	float v; \
	list<record>::iterator i; \
	pthread_mutex_lock(&memMutex); \
		for (i = recordsToFile.begin(); i != recordsToFile.end(); i++) \
		{ \
			t = i->t; \
			v = i->v; \
			__VA_ARGS__ \
		} \
	pthread_mutex_unlock(&memMutex); \
}

#define FOR_ALL(...) FOR_ALL_IN_FILE(__VA_ARGS__); FOR_ALL_IN_MEM(__VA_ARGS__)
#define RSIZE ( sizeof(double) + sizeof(float) )

floatLog::floatLog(string pan):pathAndName(pan) {}

floatLog::~floatLog() 
{
	writeToFile(); 
}

void floatLog::append(double t, float v)
{
	pthread_mutex_lock(&memMutex);
		recordsToFile.insert(recordsToFile.end(), {t,v});
	pthread_mutex_unlock(&memMutex);
}

void floatLog::readBinary(map<double, float> &data)
{
	FOR_ALL(data[t] = v; );
}	

void floatLog::readFromTo(map<double, float> &data, double from, double to)
{
	FOR_ALL( if (t >= from and t <= to) data[t] = v; );
}	

// Read one value at a specific time. Take the last previous for this value. JCE, 19-6-2019
bool floatLog::get_value_at(double time, float &value, double &valtime)
{
	bool found = false;
	double _valtime;
	FOR_ALL( if (t <= time and t > _valtime) { found = true; _valtime = t; value = v; } );
	pthread_mutex_unlock(&memMutex);
	valtime = _valtime;
	return found;
}

// function to make a summary of the measurement data without using overhead memory and time to create a list or map with all measurement values.
// parameter 1: stats array. Should be of length (paramer 2)
void floatLog::summaryFromTo(vector<flStat> &stats, unsigned bins, double from, double to)
{
	double interval =(( double) to - from) / bins;
	#define bin(t) 0 + (t-from) / interval

	// Set the stats to a known value
	for(unsigned i = 0; i<bins; i++)
	{
		stats[i].nr = 0;
		stats[i].min = 0;
		stats[i].avg = 0;
		stats[i].max = 0;
	}

	// This fills the stats.avg as the sum of stats.nr measurement values.
	// And fills stats.min, stats.max.

	FOR_ALL(	if (t >= from and t <= to and isfinite(v)) \
				{ \
					stats[bin(t)].avg += v; \
					stats[bin(t)].nr ++; \
					if (v < stats[bin(t)].min or stats[bin(t)].nr == 1) \
						stats[bin(t)].min = v; \
					if (v > stats[bin(t)].max or stats[bin(t)].nr == 1) \
						stats[bin(t)].max = v; \
					} );

	// Calculate the average
	for(unsigned i = 0; i<bins; i++)
		if (stats[i].nr)
			stats[i].avg = stats[i].avg / stats[i].nr;
}

record floatLog::getLast()
{
	FILE *fp;
	record r = {0, 0};
	pthread_mutex_lock(&fileMutex);
	fp = fopen(pathAndName.c_str(), "r");
		if (fp)
		{
			// seek 1 record before end
			fseek(fp, - RSIZE, SEEK_END);
			// read record
			fread(&r.t, sizeof(r.t), 1, fp);
			fread(&r.v, sizeof(r.v), 1, fp) ;
			fclose(fp);
		}
	pthread_mutex_unlock(&fileMutex);
	return r;
}

void floatLog::writeToFile() 
{
	list<record>::iterator i;
	FILE *fp;
	pthread_mutex_lock(&fileMutex);
	pthread_mutex_lock(&memMutex);

	fp = fopen(pathAndName.c_str(), "r+");
	if (!fp && errno == ENOENT)
		fp = fopen(pathAndName.c_str(), "w+"); // Apparently there is no fopen with create, read, write, and not append.

	if(fp)
    { 
        // Align to a multiple of records. Opened for appending always writes at the back so no seeking back.
        // JCE, 29-8-2019
        fseek(fp, 0, SEEK_END);
        long unsigned int pos = ftell(fp);
        unsigned int overshoot_on_multiple_of_records = pos % RSIZE;
        if (overshoot_on_multiple_of_records)
            printf("%s: Length (%ld) is not a multiple of record length(%ld). Overshoot: %ld. Starting from %ld\n", pathAndName.c_str(), pos, RSIZE, pos %  RSIZE, pos-overshoot_on_multiple_of_records);
        fseek(fp, pos-overshoot_on_multiple_of_records, SEEK_SET);  // Overwrite the end if the length is not a multiple of record. JCE, 10-12-2020

        for (i = recordsToFile.begin(); i != recordsToFile.end(); i++)
		{
            fwrite(& (*i).t, sizeof(double), 1, fp);
            fwrite(& (*i).v, sizeof(float), 1, fp);
		}
        fclose(fp);
    }
	recordsToFile.clear();
	pthread_mutex_unlock(&fileMutex);
	pthread_mutex_unlock(&memMutex);
}

void floatLog::addDataToFloatLog(map<double, float> &data)
{
	FOR_ALL_IN_MEM( data[t] = v; );
	pthread_mutex_lock(&fileMutex);
	FOR_ALL_IN_FILE_UNM( data[t] = v; );
	FILE *fp = fopen(pathAndName.c_str(), "w");
	if (fp)
	{
		for (map<double, float>::iterator i = data.begin(); i!= data.end(); ++i)
		{
			fwrite(&i->first, sizeof(i->first), 1, fp);
			fwrite(&i->second, sizeof(i->second), 1, fp);
		}
		fclose(fp);
	}
	pthread_mutex_unlock(&fileMutex);
}

// This function reads a text file with in text a double, and a float per line.
// The measurements will be added to the local (binary) measurement file.
void floatLog::importFromTextFile(string s)
{
	map<double, float> data;
	// read import file
	ifstream file(s.c_str());
	if (!file)
		return;
	string line;
	double a;
	float b;
	while (getline(file, line))
		if (sscanf(line.c_str(), "%lf %f", &a, &b) == 2)
			data[a] = b;
	file.close();	
	// clear / delete the to import file
	remove(s.c_str());	
	// Read , clear and write own data set.
	addDataToFloatLog(data);
}

// This function reads a bin file with in binary a double and float per record.
// The measurements will be added to the local (binary) measurement file.
void floatLog::importFromBinFile(string s)
{
	map<double, float> data;
	record r;
	FILE *fp;
	// read import file

	fp = fopen(s.c_str(), "r");
		if (fp)
		{
			while (fread(&r, sizeof(record), 1, fp))
			{
				data[r.t] = r.v;
			}
			fclose(fp);
		}
		else
			return;
	// clear / delete the to import file
	remove(s.c_str());	
	// Read , clear and write own data set.
	addDataToFloatLog(data);
}

size_t floatLog::records_in_file()
{
	FILE *fp;
	size_t num;
	fp = fopen(pathAndName.c_str(), "a");
	if(fp)
	{
		num = ftell(fp) / sizeof(record);
		fclose(fp);
	}
	return num;
}

size_t floatLog::getNumRecords()
{
	size_t num;
	pthread_mutex_lock(&fileMutex);
		num = records_in_file();
	pthread_mutex_unlock(&fileMutex);
	pthread_mutex_lock(&memMutex);
		num += recordsToFile.size();
	pthread_mutex_unlock(&memMutex);
	return num;
}

void floatLog::getRecords(map<double, float> &m, size_t from_total, size_t len_total)
{
	FILE *fp;
	record r;
	size_t records_in_file = floatLog::records_in_file();

	// From the total, are there any in the file, and any in the mem?
	bool any_from_file = from_total < records_in_file;					// There are records from file needed, if the first record is one of the file recors.
	bool any_from_mem = from_total + len_total > records_in_file;		// There can be mem records needed, if the last records is further away than available in the file.

	if (any_from_file)
	{
		size_t from = from_total;
		size_t to = from_total + len_total; // Not including, so records to 4 means 0, 1, 2, 3, not 4.
		if (to > records_in_file + 1)
			to = records_in_file + 1;
		size_t len = to - from;

		pthread_mutex_lock(&fileMutex);
		fp = fopen(pathAndName.c_str(), "r");
		if (fp)
		{
			fseek(fp, sizeof(record) * from, SEEK_SET);
			while (len --)
			{
				fread(&r, sizeof(record), 1, fp);
				m[r.t] = r.v;
			}
			fclose(fp);
		}
		pthread_mutex_unlock(&fileMutex);
	}

	if (any_from_mem)
	{
		size_t from;
		size_t len;
		if (from_total > records_in_file)
		{
			from = from_total - records_in_file;
			len = len_total;
		}
		else
		{
			from = 0;
			len = len_total - (records_in_file - from_total);
		}

		pthread_mutex_lock(&memMutex);
		list<record>::iterator i = recordsToFile.begin();
		while(from-- && ++i != recordsToFile.end()) {}
		while(len--)
		{
			if (i == recordsToFile.end())
				break;
			r = *i;
			m[r.t] = r.v;
			i++;
		}
		pthread_mutex_unlock(&memMutex);
	}	
}
















