#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "logic_fijnstof.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "out.h"
#include "webgui.h"
#include <list>

logic_fijnstof::logic_fijnstof(const std::string d, const std::string n) : logic(d, n){}

logic_fijnstof::~logic_fijnstof(){}

int logic_fijnstof::make_page(struct mg_connection *conn){
	mg_printf(conn, "<br>Fijnstof pagina</br><hr>");
	string line;
	in *pm10 = get_in("fijnstof_pm10");
	in *pm2 = get_in("fijnstof_pm2.5");
	in *wb = get_in("darksky_wb");
	in *ws = get_in("darksky_ws");
	in *rac = get_in("S1200_rac");
	in *rain = get_in("S1200_rain");
	if (not pm10 or not pm2 or not wb or not ws or not rain or not rac)
		return 0;
//	line = make_image_line(plotLines(pm10, pm2, wb, ws, now() - 3600, now(), 1280, 300, "last hour"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(pm10, pm2, wb, now() - 24*3600, now(), 1280, 300, "last day"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(pm10, pm2, wb, now() - 7*24*3600, now(), 1280, 300, "last week"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(pm10, pm2, wb, now() - 28*24*3600, now(), 1280, 300, "last four weeks"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(pm10, pm2, ws, now() - 24*3600, now(), 1280, 300, "last day"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(pm10, pm2, ws, now() - 7*24*3600, now(), 1280, 300, "last week"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(pm10, pm2, ws, now() - 28*24*3600, now(), 1280, 300, "last four weeks"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(pm10, pm2, rain, now() - 24*3600, now(), 1280, 300, "last day"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(pm10, pm2, rain, now() - 7*24*3600, now(), 1280, 300, "last week"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(pm10, pm2, rain, now() - 28*24*3600, now(), 1280, 300, "last four weeks"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(pm10, pm2, rac, now() - 24*3600, now(), 1280, 300, "last day"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(pm10, pm2, rac, now() - 7*24*3600, now(), 1280, 300, "last week"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(pm10, pm2, rac, now() - 28*24*3600, now(), 1280, 300, "last four weeks"));mg_printf(conn, line.c_str());
	//line = make_image_line(plotLines(pm10, pm2, wb, ws, now() - 265*24*3600, now(), 1280, 300, "last year"));mg_printf(conn, line.c_str());

	return 1;
	}


