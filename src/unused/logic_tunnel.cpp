#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "logic_tunnel.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "out.h"
#include "webgui.h"
#include <list>

using namespace std;

logic_tunnel::logic_tunnel(const string d, const string n, in* ti, in* rhi) : logic(d, n), temp(ti), rh(rhi){}


logic_tunnel::~logic_tunnel(){}

int logic_tunnel::make_page(struct mg_connection *conn){
	mg_printf(conn, "<br>Page that shows a graph with both humidity and temperature in a single graph. Intervals are 1 hour, 1 day, 1 week, 1 month and 1 year from now back.<br>");
	string line;
	line = make_image_line(plotLines(rh, temp, now() - 3600, now(), 1280, 300, "last hour"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(rh, temp, now() - 24*3600, now(), 1280, 300, "last day"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(rh, temp, now() - 7*24*3600, now(), 1280, 300, "last week"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(rh, temp, now() - 31*24*3600, now(), 1280, 300, "last month"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(rh, temp, now() - 265*24*3600, now(), 1280, 300, "last year"));mg_printf(conn, line.c_str());

	return 1;
	}


