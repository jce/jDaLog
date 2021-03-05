#include "webgui_intable.h"

#include <map>
#include "time.h"
#include "timefunc.h"

using namespace std;

// Timestamps
std::string table_fromto(in *i, double from, double to) 
{
	string rv;
	map<double, float> data;
	i->getData(data, from, to);
	for (auto d = data.begin(); d != data.end(); d++)
	{	
		rv += to_string(d->first) + "\t" + to_string(d->second) + "\n";		
	}
	return rv;
}

// Formatted date/time	
std::string table_fromto_h(in *i, double from, double to) 
{
	string rv;
	char buf[50];
	map<double, float> data;
	i->getData(data, from, to);
	for (auto d = data.begin(); d != data.end(); d++)
	{	
		write_human_time(buf, d->first);
		rv += buf;
		rv += "\t" + to_string(d->second) + "\n";		
	}
	return rv;
}


