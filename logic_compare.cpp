#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "logic_compare.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "mytime.h"	// now() jeu
#include "stringStore.h"
#include "out.h"
#include "webgui.h"
#include <list>
#include <ctime>
#include "float.h"
#include "webgui.h"

logic_compare::logic_compare(const std::string d, const std::string n, in *_in_a, in *_in_b) : logic(d, n), in_a(_in_a), in_b(_in_b)
{
}

logic_compare::~logic_compare()
{
}

int logic_compare::make_page(struct mg_connection *conn)
{
	string line;
	mg_printf(conn, "comparing:<br>\n" );
	mg_printf(conn, make_in_link(in_a).c_str());
	mg_printf(conn, " and ");
	mg_printf(conn, make_in_link(in_b).c_str());
	mg_printf(conn, "<br><hr>\n");
	line = make_image_line(plotLines(in_a, in_b, now() - 3600, now(), def_w, def_h, ""));
	mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(in_a, in_b, now() - 24*3600, now(), def_w, def_h, ""));
	mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(in_a, in_b, now() - 7*24*3600, now(), def_w, def_h, ""));
	mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(in_a, in_b, now() - 4*7*24*3600, now(), def_w, def_h, ""));
	mg_printf(conn, line.c_str());
	return 1;
}

