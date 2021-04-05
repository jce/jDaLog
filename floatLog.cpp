#include "floatLog.h"
#include "mytime.h"

#include <cmath>
#include <float.h> // DBL_MIN
#include <fstream>
#include <list>
#include <map>
#include "pthread.h"
#include <sstream>
#include "stdio.h"
#include <string>

using namespace std;

//#define DBG(...) printf(__VA_ARGS__);
#define DBG(...)

#define FOR_ALL_IN_FILE_UNM(...) \
{ \
	if (mode == to_file) \
	{ \
		openfile_read(); \
		double t; \
		float v; \
    	if (fp) \
    	{ \
    	    while ( fread(&t, sizeof(t), 1, fp) && fread(&v, sizeof(v), 1, fp) ) \
			{ \
				__VA_ARGS__ \
    	    } \
    	    fclose(fp); \
    	} \
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
			{ \
				__VA_ARGS__ \
			} \
		} \
	pthread_mutex_unlock(&memMutex); \
}

#define FOR_ALL(...) FOR_ALL_IN_FILE(__VA_ARGS__); FOR_ALL_IN_MEM(__VA_ARGS__)
#define RSIZE ( sizeof(double) + sizeof(float) )

floatLog::floatLog(string pan):pathAndName(pan) 
{
	read_last_from_file();
}

floatLog::~floatLog() 
{
	writeToFile(); 
}

void floatLog::read_last_from_file()
{
	pthread_mutex_lock(&fileMutex);
	openfile_read();
		if (fp)
		{
			// seek 1 record before end
        	fseek(fp, 0, SEEK_END);
        	size_t pos = ftell(fp) / RSIZE;
			if (pos == 0)
			{
				last.t = 0;
				last.v = 0;
				fclose(fp);
				pthread_mutex_unlock(&fileMutex);
				return;
			}
        	fseek(fp, (pos - 1) * RSIZE, SEEK_SET);
			// read record
			fread(&last.t, sizeof(double), 1, fp);
			fread(&last.v, sizeof(float), 1, fp) ;
			fclose(fp);
		}
	pthread_mutex_unlock(&fileMutex);
}

void floatLog::append(double t, float v)
{
	pthread_mutex_lock(&memMutex);
	if(t <= last.t)
	{
		printf("%s: Rejecting record %lf %f: time is not newer than last record (%lf)\n", pathAndName.c_str(), t, v, last.t);
		pthread_mutex_unlock(&memMutex);
		return;
	}
	if(t > now() + 10)
	{
		printf("%s: Rejecting record %lf %f: time is more than 10 seconds into the future\n", pathAndName.c_str(), t, v);
		pthread_mutex_unlock(&memMutex);
		return;
	}
	last.t = t;
	last.v = v;
	recordsToFile.insert(recordsToFile.end(), {t,v});

	// Trim the records in memory if appropriate.
	if (ram_max_samples)
		while (recordsToFile.size() > ram_max_samples)
			recordsToFile.erase(recordsToFile.begin());
	if (ram_max_history > 0)
		while (recordsToFile.begin()->t < t - ram_max_history)
			recordsToFile.pop_front();

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
	DBG("summaryFromTo(vector %p, bins %d, from %lf, to %lf)\n", &stats, bins, from, to);
	double interval =(( double) to - from) / bins;
	#define bin(t) 0 + (t-from) / interval

	// Set the stats to a known value
	for(unsigned i = 0; i<bins; i++)
	{
		stats[i] = {};
	}

	// This fills the stats.avg as the sum of stats.nr measurement values.
	// And fills stats.min, stats.max.

	FOR_ALL_IN_MEM(	if (t >= from and t <= to and isfinite(v)) \
				{ \
					stats[bin(t)].avg += v; \
					stats[bin(t)].nr ++; \
					if (v < stats[bin(t)].min or stats[bin(t)].nr == 1) \
						stats[bin(t)].min = v; \
					if (v > stats[bin(t)].max or stats[bin(t)].nr == 1) \
						stats[bin(t)].max = v; \
					} );

	// Seeking boundaries first.
	pthread_mutex_lock(&fileMutex);
	openfile_read();
	if(fp)
	{
		size_t istart = from_index(from);
		size_t istop = from_index(to);
		fseek(fp, istart * RSIZE, SEEK_SET);
		istop -= istart;
		double t;
		float v;
		for(; istop; istop--)
		{
    		fread(&t, sizeof(t), 1, fp);
			fread(&v, sizeof(v), 1, fp);
			if (isfinite(v))
			{
				stats[bin(t)].avg += v;
				stats[bin(t)].nr ++;
				if (v < stats[bin(t)].min or stats[bin(t)].nr == 1)
					stats[bin(t)].min = v;
				if (v > stats[bin(t)].max or stats[bin(t)].nr == 1)
					stats[bin(t)].max = v;
			}
		}
		fclose(fp);
	}
	pthread_mutex_unlock(&fileMutex);


	// Calculate the average
	for(unsigned i = 0; i<bins; i++)
		if (stats[i].nr)
			stats[i].avg = stats[i].avg / stats[i].nr;
}

void floatLog::summaryFromToWeighedN(vector<flStat> &stats, unsigned bins, double from, double to, float brk)
{
	double interval =(to - from) / bins;

	// Set the stats to a known value
	for(unsigned i = 0; i<bins; i++)
	{
		stats[i] = {};
	}

	double t1 = 0; 	// Last time
	float v1 = 0;	// Last value

	FOR_ALL_IN_MEM(	if (t1 >= from and t1 <= to and isfinite(v1) and (t-t1) < brk) \
				{ \
					stats[bin(t1)].avg += v1 * (t - t1); \
					stats[bin(t1)].nr ++; \
					stats[bin(t1)].t += t-t1; \
					if (v1 < stats[bin(t1)].min or stats[bin(t1)].nr == 1) \
						stats[bin(t1)].min = v1; \
					if (v1 > stats[bin(t1)].max or stats[bin(t1)].nr == 1) \
						stats[bin(t1)].max = v1; \
				} \
				t1 = t; \
				v1 = v; \
			); \
				
	pthread_mutex_lock(&fileMutex);
	openfile_read();
	if(fp)
	{
		size_t istart = from_index(from);
		size_t istop = from_index(to);
		fseek(fp, istart * RSIZE, SEEK_SET);
		istop -= istart;
		double t;
		float v;
		for(; istop; istop--)
		{
    		fread(&t, sizeof(t), 1, fp);
			fread(&v, sizeof(v), 1, fp);
			if (isfinite(v1) and (t-t1) < brk)
			{
				stats[bin(t1)].avg += v1 * (t - t1);
				stats[bin(t1)].nr ++;
				stats[bin(t1)].t += t-t1;
				if (v1 < stats[bin(t1)].min or stats[bin(t1)].nr == 1)
					stats[bin(t1)].min = v1;
				if (v1 > stats[bin(t1)].max or stats[bin(t1)].nr == 1)
					stats[bin(t1)].max = v1;
			}
			t1 = t;
			v1 = v;
		}
		fclose(fp);
	}
	pthread_mutex_unlock(&fileMutex);

	// Calculate the average
	for(unsigned i = 0; i<bins; i++)
		if (stats[i].nr)
			stats[i].avg = stats[i].avg / stats[i].t;
}

record floatLog::getLast()
{
	record rv;
	pthread_mutex_lock(&memMutex);
	rv = last;
	pthread_mutex_unlock(&memMutex);
	return rv;
}

void floatLog::writeToFile() 
{
	DBG("writeToFile()\n");
	if (mode != to_file)
	{
		DBG("writeToFile() aborted tue to mode != to_file.\n");
		return;
	}

	list<record>::iterator i;
	pthread_mutex_lock(&fileMutex);
	pthread_mutex_lock(&memMutex);

	openfile_write();

	if(fp)
    { 
        // Align to a multiple of records. Opened for appending always writes at the back so no seeking back.
        // JCE, 29-8-2019
        fseek(fp, 0, SEEK_END);
        size_t pos = ftell(fp) / RSIZE;
		DBG("Records in file: %ld\n", pos);
        fseek(fp, pos * RSIZE, SEEK_SET);  // Overwrite the end if the length is not a multiple of record. JCE, 10-12-2020

		DBG("Records in mem: %ld\n", recordsToFile.size());
        for (i = recordsToFile.begin(); i != recordsToFile.end(); i++)
		{
            fwrite(& (*i).t, sizeof(double), 1, fp);
            fwrite(& (*i).v, sizeof(float), 1, fp);
			DBG("written %lf: %f. New length %ld\n", (*i).t, (*i).v, ftell(fp));
		}
		DBG("Records in file: %ld\n", ftell(fp) / RSIZE);
        fclose(fp);
    }
	recordsToFile.clear();
	DBG("Records in mem: %ld\n", recordsToFile.size());
	pthread_mutex_unlock(&fileMutex);
	pthread_mutex_unlock(&memMutex);
}

void floatLog::addDataToFloatLog(map<double, float> &data)
{
	FOR_ALL_IN_MEM( data[t] = v; );
	pthread_mutex_lock(&fileMutex);
	FOR_ALL_IN_FILE_UNM( data[t] = v; );
	openfile_write();
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
	// read import file

	pthread_mutex_lock(&fileMutex);
	openfile_read();
	if (!fp)
	{
		pthread_mutex_unlock(&fileMutex);
		return;
	}
	while (fread(&r, sizeof(record), 1, fp))
	{
		data[r.t] = r.v;
	}
	fclose(fp);
	pthread_mutex_unlock(&fileMutex);
	
	// clear / delete the to import file
	remove(s.c_str());	
	// Read , clear and write own data set.
	addDataToFloatLog(data);
}

size_t floatLog::records_in_file()
{
	size_t num;
	pthread_mutex_lock(&fileMutex);
	openfile_read();
	if(fp)
	{
		fseek(fp, 0, SEEK_END);
		num = ftell(fp) / RSIZE;
		fclose(fp);
	}
	pthread_mutex_unlock(&fileMutex);
	return num;
}

size_t floatLog::getNumRecords()
{
	size_t num;
	num = records_in_file();
	pthread_mutex_lock(&memMutex);
		num += recordsToFile.size();
	pthread_mutex_unlock(&memMutex);
	return num;
}

void floatLog::getRecords(map<double, float> &m, size_t from_total, size_t len_total)
{
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
		openfile_read();
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

void floatLog::openfile_read()
{
	fp = fopen(pathAndName.c_str(), "rb");
}

void floatLog::openfile_write()
{
	fp = fopen(pathAndName.c_str(), "rb+");
	if (!fp && errno == ENOENT)
		fp = fopen(pathAndName.c_str(), "wb+"); // Apparently there is no fopen with create, read, write, and not append.
}

// File should be open, returns the first record number in the file at or above given timestamp.
size_t floatLog::from_index(double t)
{
	DBG("from_index(%f)\n", t);
	// search between 2 indexes, a and b. both can be one past the last record.
	size_t a = -1;
	fseek(fp, 0, SEEK_END);
	size_t b = ftell(fp) / RSIZE + 1;
	size_t x;
	double d;
	while ((b - a) > 1)
	{
		x = a + (b-a) / 2;
		fseek(fp, x * RSIZE, SEEK_SET);
        fread(&d, sizeof(d), 1, fp);
		DBG("%ld %ld %ld\n", a, b, x);
		if (d >= t)
			b = x;
		else
			a = x;
	}
	DBG("found: %ld\n", b);
	return b;
}

bool floatLog::file_is_ok()
{
	printf("checking %s\n", pathAndName.c_str());
	bool rv = true;
	
	// Test file length
	pthread_mutex_lock(&fileMutex);
	openfile_read();
	if (!fp)
	{
		pthread_mutex_unlock(&fileMutex);
		return true;	// No file, no contents to check.
	}
   	fseek(fp, 0, SEEK_END);
	size_t len = ftell(fp);
    size_t pos = len / RSIZE;
	rv = len == pos * RSIZE;
	fclose(fp);
	pthread_mutex_unlock(&fileMutex);
	if (!rv)
	{
		printf("%s: filelength (%zu) is not a multiple of record length (%zu).\n", pathAndName.c_str(), len, RSIZE);
		return rv;
	}

	double x = -DBL_MAX;
	size_t rec = 0;
	double n = now();
	FOR_ALL_IN_FILE(\
		if (x >= t)\
		{\
			if(rv) \
				printf("%s: file is not linear at position %zu, time %lf, prev time %lf, diff: %lf, value %f\n", pathAndName.c_str(), rec, t, x, t-x, v);\
			rv = false;\
		}\
		if (t <= 1000)\
		{\
			if(rv) \
				printf("%s: file contains record before time 1000 at position %zu, time %lf, value %f\n", pathAndName.c_str(), rec, t, v);\
			rv = false;\
		}\
		if (t >= n)\
		{\
			if(rv)\
				printf("%s: file contains record in the future at position %zu, time %lf, value %f\n", pathAndName.c_str(), rec, t, v);\
			rv = false;\
		}\
		if (! isfinite(v))\
		{\
			if(rv)\
				printf("%s: file contains record with non finite value at position %zu, time %lf, value %f\n", pathAndName.c_str(), rec, t, v);\
			rv = false;\
		}\
		x = t;\
		rec++;\
		);
	return rv;
}

// Sort the binary file, rewriting all.
void floatLog::sort_file()
{
	if (file_is_ok())
		return;
	printf("Sorting %s...\n", pathAndName.c_str());
	map<double, float> m;
	pthread_mutex_lock(&fileMutex);
	fp = fopen(pathAndName.c_str(), "rb");
	if (!fp)
	{
		printf("Reading file failed.\n");
    	pthread_mutex_unlock(&fileMutex);
		return;
	} 
    fseek(fp, 0, SEEK_END);
	fclose(fp); 
	FOR_ALL_IN_FILE_UNM(m[t] = v;); // Opens and closes the file internally
	printf("Records before:  %14zu\n", m.size()); 
	fp = fopen(pathAndName.c_str(), "wb"); // Overwriting!!
   	if (!fp)
	{
		printf("Opening file for writing failed!\n");
    	pthread_mutex_unlock(&fileMutex);
		return;
	}
	double n = now();
	size_t cnt = 0;
	if (m.size())
		for (auto i = m.begin(); i != m.end(); i++)
			if (i->first > 1000 and i->first < n and isfinite(i->second))
				{
    			    fwrite(&i->first, sizeof(double), 1, fp);
    			    fwrite(&i->second, sizeof(float), 1, fp);
					cnt++;
				}
    fclose(fp);
    pthread_mutex_unlock(&fileMutex);
	printf("Records after:   %14zu\nRecords removed: %14zu\n", cnt, m.size() - cnt); 
	printf("done\n");
}

floatLog::operationmode floatLog::get_operation_mode()
{
	return mode;
}

void floatLog::set_operation_mode(operationmode m)
{
	
	// It would be strange to have the last sample depending on the file if mode is ram_only.
	if (mode != m && m == ram_only && recordsToFile.size() == 0)
	{
		last.t = 0;
		last.v = 0;
	}

	// When switching to to_file, make sure the sequence is OK.
	if (mode != m && m == to_file)
	{
		read_last_from_file();
    	pthread_mutex_lock(&memMutex);
		while (recordsToFile.size() && recordsToFile.begin()->t < last.t)
			recordsToFile.pop_front();
		if (recordsToFile.size())
			last = recordsToFile.back();
    	pthread_mutex_unlock(&memMutex);
	}

	mode = m;
}

void floatLog::set_ram_max_samples(size_t s)
{
	ram_max_samples = s;
}

void floatLog::set_ram_max_history(float t)
{
	ram_max_history = t;
}










