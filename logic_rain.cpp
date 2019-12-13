#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "logic_rain.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "mytime.h"	// now() jeu
#include "stringStore.h"
#include "out.h"
#include "webgui.h"
#include <list>

logic_rain::logic_rain(const std::string d, const std::string n) : logic(d, n){}

logic_rain::~logic_rain(){}

int logic_rain::make_page(struct mg_connection *conn){
	mg_printf(conn, "<br>Rain page</br><hr>");
	string line;
	in *rd = get_in("S1200_rain");	// Rain detector: Conductivity check of heated panel facing the sky
	in *rc = get_in("S1200_rac");	// Quantity of rain by means of bucket tips.
	in *rw = get_in("rwl_wv");		// Quantity of rain water in storage
	in *dpi = get_in("darksky_pi");	// Darksky: NOAA precepitation intensity
	in *dpp = get_in("darksky_pp");	// Darksky: NOAA Precipitation probability
	in *dh = get_in("darksky_hum");	// Darksky: NOAA humidity
	in *fh = get_in("fijnstof_rh");	// Fijnstof sensor: Humidity
	if (not rd or not rc or not rw or not dpi or not dpp or not dh or not fh)
		return 0;
	line = make_image_line(plotLines(rc, rd, now() - 24*3600, now(), 1280, 300, "last day"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(rc, rw, now() - 24*3600, now(), 1280, 300, "last day"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(rc, dpi, now() - 24*3600, now(), 1280, 300, "last day"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(rc, dpp, now() - 24*3600, now(), 1280, 300, "last day"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(rc, dh, fh, now() - 24*3600, now(), 1280, 300, "last day"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(rc, rd, now() - 7*24*3600, now(), 1280, 300, "last week"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(rc, rw, now() - 7*24*3600, now(), 1280, 300, "last week"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(rc, dpi, now() - 7*24*3600, now(), 1280, 300, "last week"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(rc, dpp, now() - 7*24*3600, now(), 1280, 300, "last week"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(rc, dh, fh, now() - 24*3600, now(), 1280, 300, "last week"));mg_printf(conn, line.c_str());

	return 1;
	}


