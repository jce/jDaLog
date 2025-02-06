#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "logic_compare.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "out.h"
#include "webgui.h"
#include <list>
#include <ctime>
#include "float.h"
#include "webgui.h"

using namespace std;

logic_compare::logic_compare(const std::string d, const std::string n, list<in*>_il) : logic(d, n), inlist(_il)
{
}

logic_compare::~logic_compare()
{
}

int logic_compare::make_page(struct mg_connection *conn)
{
	/*
	unsigned int w = gw;
	if (!w)
		w = def_w;
	unsigned int h = gh;
	if (!h)
		h = def_h;

	string line;
	mg_printf(conn, "comparing:<br>\n" );
	for (auto i = inlist.begin(); i != inlist.end(); i++)
		mg_printf(conn, "%s ", make_in_link(*i).c_str());
	mg_printf(conn, "<br><hr>\n");
	line = make_image_line(plotLines(inlist, now() - 60, now(), w, h, ""));
	mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(inlist, now() - 3600, now(), w, h, ""));
	mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(inlist, now() - 24*3600, now(), w, h, ""));
	mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(inlist, now() - 7*24*3600, now(), w, h, ""));
	mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(inlist, now() - 4*7*24*3600, now(), w, h, ""));
	mg_printf(conn, line.c_str());
	*/
	return 1;
}

