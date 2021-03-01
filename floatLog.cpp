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


floatLog::floatLog(string pan):pathAndName(pan) {}

floatLog::~floatLog() 
{
	writeToFile(); 
}

void floatLog::append(record &r) 
{
	pthread_mutex_lock(&memMutex);
		recordsToFile.insert(recordsToFile.end(), r);
	pthread_mutex_unlock(&memMutex);
}

void floatLog::append(double t, float v)
{
	record r = {t, v};
	append(r);
}

void floatLog::read(list<record> &records, double start, double end) 
{
	FILE *fp;
	record r;
	pthread_mutex_lock(&fileMutex);
		fp = fopen(pathAndName.c_str(), "r");
		if (fp)
		{
			while (fread(&r, sizeof(record), 1, fp))
			{
				if (r.t >= start and r.t <= end)
					records.insert(records.end(), r);
			}
			fclose(fp);
		}
	pthread_mutex_unlock(&fileMutex);
	list<record>::iterator i;
	pthread_mutex_lock(&memMutex);
		for (i = recordsToFile.begin(); i != recordsToFile.end(); i++)
		{
			r = *i;
			if (r.t >= start and r.t <= end)
				records.insert(records.end(), r);
		}
	pthread_mutex_unlock(&memMutex);
}

void floatLog::readBinary(map<double, float> &data)
{
	FILE *fp;
	record r;
	pthread_mutex_lock(&fileMutex);
	fp = fopen(pathAndName.c_str(), "r");
		if (fp)
		{
			while (fread(&r, sizeof(record), 1, fp))
			{
				data[r.t] = r.v;
			}
			fclose(fp);
		}
	pthread_mutex_unlock(&fileMutex);
	list<record>::iterator i;
	pthread_mutex_lock(&memMutex);
		for (i = recordsToFile.begin(); i != recordsToFile.end(); i++)
		{
			r = *i;
			data[r.t] = r.v;
		}
	pthread_mutex_unlock(&memMutex);
}	

void floatLog::readFromTo(map<double, float> &data, double from, double to)
{
	FILE *fp;
	record r;
	pthread_mutex_lock(&fileMutex);
	fp = fopen(pathAndName.c_str(), "r");
		if (fp)
		{
			while (fread(&r, sizeof(record), 1, fp))
			{
				if (r.t >= from and r.t <= to)
					data[r.t] = r.v;
			}
			fclose(fp);
		}
	pthread_mutex_unlock(&fileMutex);
	list<record>::iterator i;
	pthread_mutex_lock(&memMutex);
		for (i = recordsToFile.begin(); i != recordsToFile.end(); i++)
		{
			r = *i;
			if (r.t >= from and r.t <= to)
				data[r.t] = r.v;
		}
	pthread_mutex_unlock(&memMutex);
}	

// Read one value at a specific time. Take the last previous for this value. JCE, 19-6-2019
bool floatLog::get_value_at(double time, float &value, double &valtime)
{
	bool found = false;
	FILE *fp;
	double _valtime;
	record r;
	pthread_mutex_lock(&fileMutex);
	fp = fopen(pathAndName.c_str(), "r");
		if (fp)
		{
			while (fread(&r, sizeof(record), 1, fp))
			{
				if (r.t <= time and r.t > _valtime)
				{
					found = true;
					_valtime = r.t;
					value = r.v;
				}
			}
			fclose(fp);
		}
	pthread_mutex_unlock(&fileMutex);
	list<record>::iterator i;
	pthread_mutex_lock(&memMutex);
		for (i = recordsToFile.begin(); i != recordsToFile.end(); i++)
		{
			r = *i;
			if (r.t <= time)
			{
				found = true;
				_valtime = i->t;
				value = i->v;
			}
			else
				break;
		}
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
	FILE *fp;
	record r;
	pthread_mutex_lock(&fileMutex);
	fp = fopen(pathAndName.c_str(), "r");
		if (fp){
			while (fread(&r, sizeof(record), 1, fp))
				if (r.t >= from and r.t <= to and isfinite(r.v)){ // Added the value is not NaN check. JCE, 23-11-14 // isfinite JCE 21-1-2015
					stats[bin(r.t)].avg += r.v;
					stats[bin(r.t)].nr ++;
					if (r.v < stats[bin(r.t)].min or stats[bin(r.t)].nr == 1)
						stats[bin(r.t)].min = r.v;					
					if (r.v > stats[bin(r.t)].max or stats[bin(r.t)].nr == 1)
						stats[bin(r.t)].max = r.v;					
					}
			fclose(fp);
			}
	pthread_mutex_unlock(&fileMutex);
	list<record>::iterator i;
	pthread_mutex_lock(&memMutex);
		for (i = recordsToFile.begin(); i != recordsToFile.end(); i++){
			r = *i;
			if (r.t >= from and r.t <= to and isfinite(r.v)){ // Added the value is not NaN check. JCE, 23-11-14
				stats[bin(r.t)].avg += r.v;
				stats[bin(r.t)].nr ++;					
				if (r.v < stats[bin(r.t)].min or stats[bin(r.t)].nr == 1)
					stats[bin(r.t)].min = r.v;					
				if (r.v > stats[bin(r.t)].max or stats[bin(r.t)].nr == 1)
					stats[bin(r.t)].max = r.v;					
				}
			}
	pthread_mutex_unlock(&memMutex);

	// Calculate the average
	for(unsigned i = 0; i<bins; i++)
		if (stats[i].nr)
			stats[i].avg = stats[i].avg / stats[i].nr;
}

float floatLog::getLastValue()
{
	FILE *fp;
	record r;
	pthread_mutex_lock(&fileMutex);
	int rv(0);
	fp = fopen(pathAndName.c_str(), "r");
		if (fp)
		{
			// seek 1 record before end
			fseek(fp, -sizeof(record), SEEK_END);
			// read record
			rv = fread(&r, sizeof(record), 1, fp);
			fclose(fp);
		}
	pthread_mutex_unlock(&fileMutex);
	if (rv == 1)
		return r.v;
	return 0;
}

double floatLog::getLastTime()
{
	FILE *fp;
	record r;
	pthread_mutex_lock(&fileMutex);
	int rv(0);
	fp = fopen(pathAndName.c_str(), "r");
		if (fp){
			fseek(fp, -sizeof(record), SEEK_END);
			rv = fread(&r, sizeof(record), 1, fp);
			fclose(fp);}
	pthread_mutex_unlock(&fileMutex);
	if (rv == 1) return r.t;
	return 0;
}

void floatLog::writeToFile() 
{
	list<record>::iterator i;
	record r;
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
        unsigned int rlen = sizeof(record);
        unsigned int overshoot_on_multiple_of_records = pos % rlen;
        if (overshoot_on_multiple_of_records)
            printf("%s: Length (%ld) is not a multiple of record length(%d). Overshoot: %ld. Starting from %ld\n", pathAndName.c_str(), pos, rlen, pos %  rlen, pos-overshoot_on_multiple_of_records);
        fseek(fp, pos-overshoot_on_multiple_of_records, SEEK_SET);  // Overwrite the end if the length is not a multiple of record. JCE, 10-12-2020

        for (i = recordsToFile.begin(); i != recordsToFile.end(); i++)
		{
            r = *i;
            fwrite(&r, sizeof(record), 1, fp);
		}
        fclose(fp);
    }
	recordsToFile.clear();
	pthread_mutex_unlock(&fileMutex);
	pthread_mutex_unlock(&memMutex);
}

void floatLog::addDataToFloatLog(map<double, float> &data)
{
	FILE *fp;
	record r;
	// lock memory / buffer
	pthread_mutex_lock(&memMutex);
		// Read to local buffer
		for (list<record>::iterator i = recordsToFile.begin(); i != recordsToFile.end(); i++)
		{
			r = *i;
			data[r.t] = r.v;
		}
		// clear memory / buffer
		recordsToFile.clear();
	// release memory / buffer lock
	pthread_mutex_unlock(&memMutex);

	// lock file mutex	
	pthread_mutex_lock(&fileMutex);
		// Read the file to data
		fp = fopen(pathAndName.c_str(), "r");
		if (fp)
		{
			while (fread(&r, sizeof(record), 1, fp))
			{
				data[r.t] = r.v;
			}
			fclose(fp);
		}
		// close the file, and reopen it for writing
		fp = fopen(pathAndName.c_str(), "w");
		if (fp)
		{
			for (map<double, float>::iterator i = data.begin(); i!= data.end(); ++i)
			{
				r.t = i->first;
				r.v = i->second;
				fwrite(&r, sizeof(record), 1, fp);
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
















