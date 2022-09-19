
#include "floatLog.h"
#include "logic_sleep.h"
#include "main.h"
#include "mytime.h"
#include "stringStore.h"
#include "webgui.h"
#include "out.h"

#include <ctime>
#include "float.h"
#include <list>
#include "stdio.h"
#include <string>
#include "string.h"
#include "sys/stat.h"
#include "sys/time.h"

using namespace std;

logic_sleep::logic_sleep(const std::string d, const std::string n, list<in*> _in) : logic(d, n), inp(_in)
{
}

logic_sleep::~logic_sleep()
{
}

const char* month_str[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

int logic_sleep::make_page(struct mg_connection *conn)
{
	mg_printf(conn, "<br>Sleep overview page<br>");
	string line;

	list<map<double, float>> datas;
	map<double, float> data;
	for( auto i=inp.begin(); i != inp.end(); i++)
	{
		data.clear();
		(*i)->getData(data, 1, DBL_MAX);
		datas.push_back(data);
	} 

	double start = DBL_MIN, end = DBL_MAX;
	if (datas.size()
	{
		for( auto i = datas.begin(); i != datas.end(); i++)
		{
			if ( i->begin()->first > start )
				start = i->begin()->first;
			if (i->end()->first < end )
				end = i->end()->first;
		}

		list<struct { int year, month, day, asdfasdf

		int year_start, month_start, day_start, year_end, month_end, day_end;
		if (start != DBL_MIN and end != DBL_MAX)
		{
			time_t ttm;
			struct tm stm;
			ttm = start;
			stm = *localtime(&ttm);
			year_start = stm.tm_year;
			month_start = stm.tm_mon;
			day_start = stm.tm_mday;
			ttm = end;
			stm = *localtime(&ttm);
			year_end = stm.tm_year;
			month_end = stm.tm_mon;
			day_end = stm.tm_mday;
		}

	int year, month, day;
	for (year = year_start; year <= year_end; year++)
	{
		int ms = 0;
		int me = 11;
		if (year == year_start)
			ms = month_start;
		if (year == year_end)
			me = month_end;
		for (month = ms; month <= me; month++)
		{
			struct tm from = {};
			from.tm_year = year;
			from.tm_mon = month;
			from.tm_mday = 1;
			time_t from_ts = mktime(&from);
			struct tm to = {};
			to.tm_year = year;
			to.tm_mon = month + 1;
			to.tm_mday = 1;
			time_t to_ts = mktime(&to);

			line = make_image_line(plotLines(inp, from_ts, to_ts, 1280, 300, month_str[month]));
			mg_printf(conn, line.c_str());

		}
	}

	return 1;
}

