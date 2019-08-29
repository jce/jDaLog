#include "stdio.h"
#include <list>
#include <string>
#include <map>
#include "pthread.h"
#include <fstream>
#include <sstream>
#include <cmath>

#include "floatLog.h"
using namespace std;

//#define debug

floatLog::floatLog(string pan):_pathAndName(pan) {
	//pathAndName = pan;
	//printf("Initing mutexes...\n");
	pthread_mutex_init(&_fileMutex, NULL);
	//printf("have one...\n");
	pthread_mutex_init(&_memMutex, NULL); }
	//printf("mutex inited\n");}

floatLog::~floatLog() {
	writeToFile(); }

void floatLog::append(list<record> &) {}
	//recordsToFile.insert(recordsToFile.end()

void floatLog::append(record &r) {
	pthread_mutex_lock(&_memMutex);
		_recordsToFile.insert(_recordsToFile.end(), r);
	pthread_mutex_unlock(&_memMutex);}

void floatLog::append(double t, float v) {
	record r = {t, v};
	append(r);}

void floatLog::read(list<record> &records, double start, double end) {
	FILE *fp;
	record r;
	pthread_mutex_lock(&_fileMutex);
		fp = fopen(_pathAndName.c_str(), "r");
		if (fp){
			while (fread(&r, sizeof(record), 1, fp)){
				if (r.t >= start and r.t <= end)
					records.insert(records.end(), r);}
			fclose(fp);}
	pthread_mutex_unlock(&_fileMutex);
	list<record>::iterator i;
	pthread_mutex_lock(&_memMutex);
		for (i = _recordsToFile.begin(); i != _recordsToFile.end(); i++){
			r = *i;
			if (r.t >= start and r.t <= end)
				records.insert(records.end(), r);}
	pthread_mutex_unlock(&_memMutex);
	}

void floatLog::readBinary(map<double, float> &data){
	FILE *fp;
	record r;
	pthread_mutex_lock(&_fileMutex);
	fp = fopen(_pathAndName.c_str(), "r");
		if (fp){
			while (fread(&r, sizeof(record), 1, fp)){
				data[r.t] = r.v;}
			fclose(fp);}
	pthread_mutex_unlock(&_fileMutex);
	list<record>::iterator i;
	pthread_mutex_lock(&_memMutex);
		for (i = _recordsToFile.begin(); i != _recordsToFile.end(); i++){
			r = *i;
			data[r.t] = r.v;}
	pthread_mutex_unlock(&_memMutex);}	

void floatLog::readFromTo(map<double, float> &data, double from, double to){
	//printf("Please dont use floatlog::readFromTo(blah, blah) for plotting");
	FILE *fp;
	record r;
	pthread_mutex_lock(&_fileMutex);
	fp = fopen(_pathAndName.c_str(), "r");
		if (fp){
			while (fread(&r, sizeof(record), 1, fp)){
				if (r.t >= from and r.t <= to)
					data[r.t] = r.v;}
			fclose(fp);}
	pthread_mutex_unlock(&_fileMutex);
	list<record>::iterator i;
	pthread_mutex_lock(&_memMutex);
		for (i = _recordsToFile.begin(); i != _recordsToFile.end(); i++){
			r = *i;
			if (r.t >= from and r.t <= to)
				data[r.t] = r.v;}
	pthread_mutex_unlock(&_memMutex);}	

// Read one value at a specific time. Take the last previous for this value. JCE, 19-6-2019
bool floatLog::get_value_at(double time, float &value, double &valtime)
{
	bool found = false;
	FILE *fp;
	double _valtime;
	//double dist = -1; // Time distance of last measurement time with the required time.
	record r;
	pthread_mutex_lock(&_fileMutex);
	fp = fopen(_pathAndName.c_str(), "r");
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
				//else		// It might be that the values are not always chronologically ordened. JCE, 20-6-2019
				//	break;
			}
			fclose(fp);
		}
	pthread_mutex_unlock(&_fileMutex);
	list<record>::iterator i;
	pthread_mutex_lock(&_memMutex);
		for (i = _recordsToFile.begin(); i != _recordsToFile.end(); i++)
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
	pthread_mutex_unlock(&_memMutex);
	
	valtime = _valtime;
	return found;
}

// function to make a summary of the measurement data without using overhead memory and time to create a list or map with all measurement values.
// parameter 1: stats array. Should be of length (paramer 2)
void floatLog::summaryFromTo(vector<flStat> &stats, unsigned bins, double from, double to){	// JCE, 30-12-13
	double interval =(( double) to - from) / bins;
	#define bin(t) 0 + (t-from) / interval

	// Set the stats to a known value
	for(unsigned i = 0; i<bins; i++){
		stats[i].nr = 0;
		stats[i].min = 0;
		stats[i].avg = 0;
		stats[i].max = 0;}

	// Brute copy paste action ftl... wat wil je ook met een noob als programmeur
	// This fills the stats.avg as the sum of stats.nr measurement values.
	// And fills stats.min, stats.max.
	FILE *fp;
	record r;
	pthread_mutex_lock(&_fileMutex);
	fp = fopen(_pathAndName.c_str(), "r");
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
	pthread_mutex_unlock(&_fileMutex);
	list<record>::iterator i;
	pthread_mutex_lock(&_memMutex);
		for (i = _recordsToFile.begin(); i != _recordsToFile.end(); i++){
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
	pthread_mutex_unlock(&_memMutex);

	// Calculate the average
	for(unsigned i = 0; i<bins; i++)
		if (stats[i].nr)
			stats[i].avg = stats[i].avg / stats[i].nr;
	}

float floatLog::getLastValue(){
	FILE *fp;
	record r;
	pthread_mutex_lock(&_fileMutex);
	int rv(0);
	fp = fopen(_pathAndName.c_str(), "r");
		if (fp){
			// seek 1 record before end
			fseek(fp, -sizeof(record), SEEK_END);
			// read record
			rv = fread(&r, sizeof(record), 1, fp);
			fclose(fp);}
	pthread_mutex_unlock(&_fileMutex);
	if (rv == 1) return r.v;
	return 0;}

double floatLog::getLastTime(){
	FILE *fp;
	record r;
	pthread_mutex_lock(&_fileMutex);
	int rv(0);
	fp = fopen(_pathAndName.c_str(), "r");
		if (fp){
			fseek(fp, -sizeof(record), SEEK_END);
			rv = fread(&r, sizeof(record), 1, fp);
			fclose(fp);}
	pthread_mutex_unlock(&_fileMutex);
	if (rv == 1) return r.t;
	return 0;}

void floatLog::writeToFile() {
	//printf("writing to file\n");
	list<record>::iterator i;
	record r;
	FILE *fp;
	//printf("opening file for appending");
	pthread_mutex_lock(&_fileMutex);
	pthread_mutex_lock(&_memMutex);
		fp = fopen(_pathAndName.c_str(), "a");

		// Align to a multiple of records. Opened for appending always writes at the back so no seeking back.
		// JCE, 29-8-2019
		long int pos = ftell(fp);
		unsigned int rlen = sizeof(record);
		unsigned int overshoot_on_multiple_of_records = pos % rlen;
		if (overshoot_on_multiple_of_records)
		{
			int padding_required = rlen - overshoot_on_multiple_of_records;
			printf("%s is not record aligned, adding %i bytes.\n", _pathAndName.c_str(), padding_required);
			for (int i = 0; i < padding_required; i++)
				fputc('\0' , fp);
		}
		for (i = _recordsToFile.begin(); i != _recordsToFile.end(); i++){
			r = *i;
			fwrite(&r, sizeof(record), 1, fp);}
		fclose(fp);
		_recordsToFile.clear();
	pthread_mutex_unlock(&_fileMutex);
	pthread_mutex_unlock(&_memMutex);}

void floatLog::_addDataToFloatLog(map<double, float> &data){
	FILE *fp;
	record r;
	// lock memory / buffer
	#ifdef debug
		printf("Reading floatLog memory and binary file...\n");
	#endif
	pthread_mutex_lock(&_memMutex);
		// Read to local buffer
		for (list<record>::iterator i = _recordsToFile.begin(); i != _recordsToFile.end(); i++){
			r = *i;
			data[r.t] = r.v;}
		// clear memory / buffer
		_recordsToFile.clear();
	// release memory / buffer lock
	pthread_mutex_unlock(&_memMutex);

	// lock file mutex	
	pthread_mutex_lock(&_fileMutex);
		// Read the file to data
		fp = fopen(_pathAndName.c_str(), "r");
		if (fp){
			while (fread(&r, sizeof(record), 1, fp)){
				data[r.t] = r.v;}
			fclose(fp);}
		// close the file, and reopen it for writing
		#ifdef debug
			printf("Overwriting floatLog binary file...\n");
		#endif
		fp = fopen(_pathAndName.c_str(), "w");
		if (fp){
			#ifdef debug
				printf("Writing floatlog file\n");
			#endif
			//for (list<record>::iterator i = recordList.begin(); i != recordList.end(); i++){
			for (map<double, float>::iterator i = data.begin(); i!= data.end(); ++i){
				r.t = i->first;
				r.v = i->second;
				fwrite(&r, sizeof(record), 1, fp);}
			fclose(fp);
			#ifdef debug
				printf("Finished\n");
			#endif
			}
	pthread_mutex_unlock(&_fileMutex);}

// This function reads a text file with in text a double, and a float per line.
// The measurements will be added to the local (binary) measurement file.
void floatLog::importFromTextFile(string s){
	map<double, float> data;
	// read import file
	#ifdef debug
		printf("Looking for text file to import...%s\n", _pathAndName.c_str());
	#endif
	ifstream file(s.c_str());
	if (!file){
		#ifdef debug
			printf("no file found\n");
		#endif
		return;}
	string line;
	double a;
	float b;
	#ifdef debug
		printf("Reading text file...\n");
	#endif
	while (getline(file, line))
		if (sscanf(line.c_str(), "%lf %f", &a, &b) == 2)
			data[a] = b;
	file.close();	
	// clear / delete the to import file
	remove(s.c_str());	
	// Read , clear and write own data set.
	_addDataToFloatLog(data);}

// This function reads a bin file with in binary a double and float per record.
// The measurements will be added to the local (binary) measurement file.
void floatLog::importFromBinFile(string s){
	map<double, float> data;
	record r;
	FILE *fp;
	// read import file
	#ifdef debug
		printf("Looking for bin file to import...%s\n", _pathAndName.c_str());
	#endif

	fp = fopen(s.c_str(), "r");
		if (fp){
			#ifdef debug
				printf("Reading text file...\n");
			#endif
			while (fread(&r, sizeof(record), 1, fp)){
				data[r.t] = r.v;}
			fclose(fp);}
		else{
			#ifdef debug
				printf("no file found\n");
			#endif
			return;}
	// clear / delete the to import file
	remove(s.c_str());	
	// Read , clear and write own data set.
	_addDataToFloatLog(data);}



















