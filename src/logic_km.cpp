#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "logic_km.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "out.h"
#include "webgui.h"
#include <list>
#include <ctime>
#include "float.h"

logic_km::logic_km(const std::string d, const std::string n, in *_in) : logic(d, n), inp(_in)
{
}

logic_km::~logic_km()
{
}

const char* month_str[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

int logic_km::make_page(struct mg_connection *conn){
	/*

	mg_printf(conn, "<br>Kilometer vergoeding pagina</br>based on ");
	mg_printf(conn, inp->getName().c_str());
	mg_printf(conn, "<br><hr>");
	string line;

	map<double, float> data;	
	inp->getData(data, 1, DBL_MAX); 

	int year_start, month_start, year_end, month_end;
	if (data.size())
	{
		time_t ttm;
		struct tm stm;
		ttm = data.begin()->first;
		stm = *localtime(&ttm);
		year_start = stm.tm_year;
		month_start = stm.tm_mon;
		ttm = data.rbegin()->first;
		stm = *localtime(&ttm);
		year_end = stm.tm_year;
		month_end = stm.tm_mon;
	}

	int year, month;
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
*/
	return 1;
}

