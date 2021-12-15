#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "logic_power.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "mytime.h"	// now() jeu
#include "stringStore.h"
#include "out.h"
#include "webgui.h"
#include <list>

logic_power::logic_power(const std::string d, const std::string n) : logic(d, n){}

logic_power::~logic_power(){}

int logic_power::make_page(struct mg_connection *conn){
	mg_printf(conn, "<br>Power page</br><hr>");
	string line;
	in *pwrsum = get_in("pwrsum");
	in *kWhsum = get_in("kwhsum");
	in *hs110_rm_p = get_in("hs110_rm_p");
	in *hs110_rm_u = get_in("hs110_rm_u");
	in *hs110_rm_tot = get_in("hs110_rm_tot");
	in *hs110_fr_p = get_in("hs110_fr_p");
	in *hs110_fr_u = get_in("hs110_fr_u");
	in *hs110_fr_tot = get_in("hs110_fr_tot");
	in *hs110_wp_p = get_in("hs110_wp_p");
	in *hs110_wp_u = get_in("hs110_wp_u");
	in *hs110_wp_tot = get_in("hs110_wp_tot");

	if (not (pwrsum and kWhsum and hs110_rm_p and hs110_rm_u and hs110_rm_tot and hs110_fr_p and hs110_fr_u and hs110_fr_tot and hs110_wp_p and hs110_wp_u and hs110_wp_tot ) )
		return 0;
	line = make_image_line(plotLines(pwrsum, kWhsum, now() - 3600, now(), 1220, 300, "last hour"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(hs110_rm_u, hs110_wp_u, hs110_fr_u, now() - 3600, now(), 1220, 300, "last hour"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(hs110_rm_p, hs110_wp_p, hs110_fr_p, now() - 3600, now(), 1220, 300, "last hour"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(pwrsum, kWhsum, now() - 24*3600, now(), 1220, 300, "last day"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(hs110_rm_u, hs110_wp_u, hs110_fr_u, now() - 24*3600, now(), 1220, 300, "last day"));mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(hs110_rm_p, hs110_wp_p, hs110_fr_p, now() - 24*3600, now(), 1220, 300, "last day"));mg_printf(conn, line.c_str());
	//line = make_image_line(plotLines(pm10, pm2, wb, now() - 7*24*3600, now(), 1280, 300, "last week"));mg_printf(conn, line.c_str());
	//line = make_image_line(plotLines(pm10, pm2, wb, now() - 28*24*3600, now(), 1280, 300, "last four weeks"));mg_printf(conn, line.c_str());
	//line = make_image_line(plotLines(pm10, pm2, ws, now() - 24*3600, now(), 1280, 300, "last day"));mg_printf(conn, line.c_str());
	//line = make_image_line(plotLines(pm10, pm2, ws, now() - 7*24*3600, now(), 1280, 300, "last week"));mg_printf(conn, line.c_str());
	//line = make_image_line(plotLines(pm10, pm2, ws, now() - 28*24*3600, now(), 1280, 300, "last four weeks"));mg_printf(conn, line.c_str());
	//line = make_image_line(plotLines(pm10, pm2, rain, now() - 24*3600, now(), 1280, 300, "last day"));mg_printf(conn, line.c_str());
	//line = make_image_line(plotLines(pm10, pm2, rain, now() - 7*24*3600, now(), 1280, 300, "last week"));mg_printf(conn, line.c_str());
	//line = make_image_line(plotLines(pm10, pm2, rain, now() - 28*24*3600, now(), 1280, 300, "last four weeks"));mg_printf(conn, line.c_str());
	//line = make_image_line(plotLines(pm10, pm2, rac, now() - 24*3600, now(), 1280, 300, "last day"));mg_printf(conn, line.c_str());
	//line = make_image_line(plotLines(pm10, pm2, rac, now() - 7*24*3600, now(), 1280, 300, "last week"));mg_printf(conn, line.c_str());
	//line = make_image_line(plotLines(pm10, pm2, rac, now() - 28*24*3600, now(), 1280, 300, "last four weeks"));mg_printf(conn, line.c_str());
	//line = make_image_line(plotLines(pm10, pm2, wb, ws, now() - 265*24*3600, now(), 1280, 300, "last year"));mg_printf(conn, line.c_str());
	return 1;
	}


