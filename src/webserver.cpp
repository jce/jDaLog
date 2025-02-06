
#include "equation.h"
#include "floatLog.h"
#include "git_header.h"
#include "in.h"
#include "in_equation.h"
#include "logic.h"
#include "main.h"
#include <microhttpd.h>
#include "out.h"
#include "stringStore.h"
#include "timefunc.h"
#include "webserver.h"
#include "webgui_intable.h"
#include "webgui_page.h"
#include "webin.h"

#include <cstdlib> // system()
#include <inttypes.h> // Macros like SCNu16
#include <list>
#include <math.h> // floor()
#include <set>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sys/stat.h> // mkdir
#include <vector>

#define DBG(...) {printf(__VA_ARGS__); printf("\n");}
//#define DBG(...)

using namespace std;


webserver::webserver(string n, uint16_t t, uint16_t p): name(n), threads(t), port(p)
{
	daemon = NULL;
	requests = new in(name + "_rq", "Webserver requests", "");
}

webserver::~webserver()
{
	if(daemon)
		stop();
	delete requests;
}

// Default handler, https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html#microhttpd_002dcb
enum MHD_Result dh
	(
    	void *cls,
    	struct MHD_Connection *connection,
    	const char *url,
    	const char *method,
    	const char *version,
    	const char *upload_data,
    	long unsigned int *upload_data_size,
    	void **con_cls
	)
{
	webserver *obj = (webserver*) cls;
	return obj->handle_request(connection, url, method, version, upload_data, upload_data_size, con_cls);
}

void webserver::start()
{
	daemon= MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD, port, NULL, NULL, &dh, this, MHD_OPTION_END);
	DBG("Webserver %s started on port %i", name.c_str(), port);
}

void webserver::stop()
{
	if(daemon)
	{
		MHD_stop_daemon(daemon);
		DBG("Stopped webserver %s", name.c_str());
	}
}


enum MHD_Result webserver::handle_request
	(
    	struct MHD_Connection *connection,
    	const char *url,
    	const char *method,
    	const char *version,
    	const char *upload_data,
    	long unsigned int *upload_data_size,
    	void **con_cls
	)
{
    const char *page = "<html><body>Hello, browser!</body></html>";
    struct MHD_Response *response;
    MHD_Result ret;

    response = MHD_create_response_from_buffer(strlen (page), (void*) page, MHD_RESPMEM_PERSISTENT);

    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;

}


















uint16_t def_w = 1000;
uint16_t def_h = 300;

uint16_t plotnr = 0;

// The purpose of this file is to control libMicroHttpD (MHD), and create a bunch of semi-uniform webpages that will serve as GUI for tcFarmControl. JCE, 14-6-13

//============================================================================================================
// This is a list with files and eol times. When a file's eol has passed, it should be deleted.
struct fl{string filename;double eol;};
list<struct fl> files;

void setFileLife(string file, double eol){
	struct fl fileLife;
	fileLife.filename = file;
	fileLife.eol = eol;
	files.push_back(fileLife);}

void deleteAllFiles(){
	list<struct fl>::iterator i;
	for (i = files.begin(); i!= files.end(); i++)
		remove(i->filename.c_str());
	files.clear();}

void deleteOldFiles(){
	double t = now();
	list<struct fl>::iterator i;
	for (i = files.begin(); i!= files.end(); ){
		if (t > i->eol){
			remove(i->filename.c_str());
			i = files.erase(i);}

		else i++;}}
//=============================================================================================================

void fprintt(FILE *fp, double a, float cycle = 0)
{
	if (cycle == 0)	// Clock time
	{
		time_t b = a;
		struct tm t;
		t = {};
		localtime_r(&b, &t);			// Added "_r". JCE, 2-10-2018
		int ms = (a - floor(a)) * 1000; // Assumption: Time is always aligned on the second... JCE, 14-9-2018
		fprintf(fp, "%u-%u-%u_%u:%u:%u.%03u", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, ms);
	}
	else // Cyclic time
	{
		int msec = fmod(a * 1000, 1);
		a = a/ 1;
		int sec = fmod(a, 60);
		a = a / 60;
		int min = fmod(a, 60);
		a = a / 60;
		int hour = fmod(a, 24);
		int day = a / 24 + 1;
		if (cycle >= 24*60*60)
			fprintf(fp, "%u_%u:%u:%u.%03u", day, hour, min, sec, msec);
		else if (cycle >= 60*60)
			fprintf(fp, "%u:%u:%u.%03u", hour, min, sec, msec);
		else if (cycle >= 60)
			fprintf(fp, "%u:%u.%03u", min, sec, msec);
		else
			fprintf(fp, "%u.%03u", sec, msec);
	} 
}

//struct stats{int nr; float min; double sum; float max;};
struct inWithData
{
	in* inp; 
	vector<flStat> stats; 
	unsigned statLen; 
	bool haveData;
	float valid_time;
	double from, to;
};

struct y_ax
{
	string units;
	list<inWithData> iwdl;
	double ymin;
	double yrange;
	double ymax;
	bool haveData;
};

// Plots a graph of given ins, between time tmin and tmax, with a width
// of x and heigth of y. The graph is stored in a specific location
// and the url to this location is returned.
string plotLines(list<in*> ins, double tmin, double tmax, unsigned int x, unsigned int y, string title, bool have_legend)
{
	// Construct the image's filename, secondly get the units for x1 and x2, and number of plottable lines.
	char imageName[1025], scriptName[1025], imageUrl[1025];
	string names;
	list<in*>::iterator ini;
	list<inWithData>::iterator iwdli; 	// In with data list iterator
	bool haveKey = ins.size() > 1 and have_legend; // "Key" is Gnuplot language for "Legend", so it uses some graph-space.
	double bins = x * 0.7;				// Approximately one bin per pixel width of the graph.
	if (haveKey) bins = x * 0.4;

	uint16_t pn = plotnr++; // Note the omission of proper locking. I could say that this will soon be fixed, but in reality i wish to migrate to another webserver. I'll just leave it untill then. Calculated risk. Well, guessed to be managable risk.
	snprintf(imageName, 1024, "http/graphs/%x.png", pn);
	snprintf(scriptName, 1024, "http/graphs/%x.gps", pn);
	snprintf(imageUrl, 1024, "/graphs/%x.png", pn);
	
	// Leave if file already exists.
	FILE* fp = fopen(imageName, "r");
	if (fp)
	{
		fclose(fp);
		return imageUrl;
	}
	string path = "http/";	
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	path += "graphs/";
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	fp = fopen(scriptName, "w");
	if (fp)
	{
		// Deel de ins in, in een lijst met y-assen. Vooral ter behoeve van berekenen van yrange
		list<y_ax> y_axes;
		list<y_ax>::iterator yi;
		for(ini=ins.begin(); ini!=ins.end(); ini++)
		{
			string units;
			units = (*ini)->getUnits();
			bool foundyax(false);
			for(yi=y_axes.begin(); yi!=y_axes.end(); yi++)
				if (yi->units == units)
				{		
					inWithData iwd;
					iwd.inp = (*ini);
					iwd.stats.resize(bins);
					iwd.statLen = bins;
					iwd.valid_time = (*ini)->get_valid_time();
					iwd.from = tmin - iwd.valid_time;
					iwd.to = tmax + iwd.valid_time;
					(*ini)->getDataSummary(iwd.stats, iwd.statLen, iwd.from, iwd.to);
					yi->iwdl.push_back(iwd);
					foundyax = true;
				}
			if (not foundyax)
			{
				inWithData iwd;
				iwd.inp = (*ini);
				iwd.stats.resize(bins);
				iwd.statLen = bins;
				iwd.valid_time = (*ini)->get_valid_time();
				iwd.from = tmin - iwd.valid_time;
				iwd.to = tmax + iwd.valid_time;
				(*ini)->getDataSummary(iwd.stats, iwd.statLen, iwd.from, iwd.to);
				list<inWithData> iwdl;								
				iwdl.push_back(iwd);
				y_ax y;
				y.units = units;
				y.iwdl = iwdl;
				y.ymin =  -1;
				y.yrange = 2;
				y.ymax = 1;
				y.haveData = false;
				y_axes.push_back(y);
			}
		}
		// Nu is er een lijst met y-assen. Per y-as moet de ymin, yrange en ymax berekend worden.
		for(yi=y_axes.begin(); yi!=y_axes.end(); yi++)
		{
			for(iwdli=yi->iwdl.begin(); iwdli!=yi->iwdl.end(); iwdli++)
			{
				for(unsigned i = 0; i < iwdli->statLen; i++)
				{
					if (iwdli->stats[i].nr)
					{
						iwdli->haveData = true;
						if (yi->haveData == false)
						{
							yi->ymin = iwdli->stats[i].min;
							yi->ymax = iwdli->stats[i].max;
							yi->haveData = true;
						}
						else
						{
							if (yi->ymin > iwdli->stats[i].min)
								yi->ymin = iwdli->stats[i].min;
							if (yi->ymax < iwdli->stats[i].max)
								yi->ymax = iwdli->stats[i].max;
						}									
					}
				}
			}
			// ymin and ymax are now known, move them away from limits a little
			if (yi->ymax != yi->ymin)
			{ // range > 0 -> ymax != ymin. JCE, 16-7-13
				yi->yrange = yi->ymax - yi->ymin;
				yi->ymin -= 0.05 * yi->yrange;
				yi->ymax += 0.05 * yi->yrange;
			}
			else
			{	// ymin == ymax
				double margin(1);
				if (yi->ymax > 1000)
					margin = 0.05 * yi->ymax;
				yi->ymin -= margin;
				yi->ymax += margin;
			}
		}

		// Deze implementatie is voor(lopig) voor 2 y-assen. Nu moet gekeken worden of er 1 zijn of meer dan 1
		bool haveY2(y_axes.size() > 1);
		y_ax y1, y2;
		y1 = *y_axes.begin();
		if (haveY2) y2 = (*(++y_axes.begin()));

		// Het opbouwen van het gnuplot bestand			
		fprintf(fp, "# gnuplot script generated by tcFarmControl " GIT_SHORT_WORDHASH_WITH_MODIFIED " (" GIT_SHORT_HASH ")\n");
		fprintf(fp, "set terminal png size %u,%u truecolor\n", x, y); // truecolor option creates anti-aliasing. JCE< 5-7-13
		fprintf(fp, "set output \"%s\"\n", imageName);
		if (title != "")
			fprintf(fp, "set title \"%s\"\n", title.c_str());
		fprintf(fp, "set xdata time\n");
		fprintf(fp, "set timefmt \"%%Y-%%m-%%d_%%H:%%M:%%S\"\n");
		fprintf(fp, "set xrange [\"");
		fprintt(fp, tmin);
		fprintf(fp, "\":\"");
		fprintt(fp, tmax);
		fprintf(fp, "\"]\n");		

		fprintf(fp, "set format x \"");
		if ( tmax-tmin > 24*60*60 - 10	) fprintf(fp, "%%d-%%m");
		if ( tmax-tmin > 364*24*60*60 	) fprintf(fp, "-%%Y");
		if ( tmax-tmin > 24*60*60 - 10	) fprintf(fp, " ");
		if ( tmax-tmin > 60 			) fprintf(fp, "%%H:%%M");
		if ( tmax-tmin <= 3600 - 10 	) fprintf(fp, ":%%S\"\n");
		fprintf(fp, "\n");		

		fprintf(fp, "set yrange [%.8f:%.8f]\n", y1.ymin, y1.ymax);
		fprintf(fp, "set ylabel \"%s\"\n", y1.units.c_str());
		if (haveY2){ 	fprintf(fp, "set y2range [%.8f:%.8f]\n", y2.ymin, y2.ymax);
				fprintf(fp, "set y2label \"%s\"\n", y2.units.c_str());
				fprintf(fp, "set y2tics\n");}
		if (haveKey)	fprintf(fp, "set key out vert right\n");
		else		fprintf(fp, "set key off\n");
		fprintf(fp, "set grid\n");
		fprintf(fp, "set style fill transparent solid 0.25 noborder\n");
		string linecolor[] = {"blue", "red", "black", "green", "magenta", "brown", "spring-green", "cyan", "dark-orange", "violet", "sienna4", "steelblue", "dark-spring-green", "goldenrod"};
		unsigned int colornr(0);
		string plotline;
		bool first = true;
		for(yi=y_axes.begin(); yi!=y_axes.end(); yi++)
		{
			for(iwdli=yi->iwdl.begin(); iwdli!=yi->iwdl.end(); iwdli++)
			{
				// Deze moet getekend. Plot line
				if (first)
				{ 
					plotline += "plot\t";
					first = false;
				}
				else			
					plotline += ", \\\n\t";
				plotline += "\"-\" using 1:2:3";
				if (haveKey) plotline += " t '" + iwdli->inp->getName() + " [" + iwdli->inp->getUnits() + "]'";
				plotline += " with filledcu lc rgb \""+linecolor[colornr]+"\"";
				if (haveY2)
				{
					plotline += " axes ";
					if (iwdli->inp->getUnits() == y1.units)
						plotline += "x1y1";
					else
						plotline += "x1y2";
				}
				plotline += ", \\\n\t\"\" using 1:2";
				if (haveKey) plotline += " t '" + iwdli->inp->getName() + " [" + iwdli->inp->getUnits() + "]'";
				plotline += " with lines lc rgb \""+linecolor[colornr]+"\"";
				if (haveY2)
				{
					plotline += " axes ";
					if (iwdli->inp->getUnits() == y1.units)
						plotline += "x1y1";
					else
						plotline += "x1y2";
				}
				colornr++; if (colornr >= 14) colornr = 0;
			}
		}
		plotline += "\n";
		fprintf(fp, plotline.c_str());

		for(yi=y_axes.begin(); yi!=y_axes.end(); yi++)
		{
			for(iwdli=yi->iwdl.begin(); iwdli!=yi->iwdl.end(); iwdli++)
			{	
				double interval = (iwdli->to - iwdli->from) / bins;
				double t;
				// neerschrijven in de gnuplot script van data range
				if (yi != y_axes.begin() or iwdli != yi->iwdl.begin())
					fprintf(fp, "e\n");	
				double prevt = 0;		
				for (unsigned i = 0; i <  iwdli->statLen; i++)
				{
					if (iwdli->stats[i].nr)
					{			
						t = iwdli->from + i * interval;
						if (prevt and t - prevt > iwdli->valid_time and t - prevt > (1.1 * interval))
							fprintf(fp, "\n");
						prevt = t;
						fprintt(fp, t + interval / 2);
						fprintf(fp, " %.8f %.8f\n", iwdli->stats[i].min, iwdli->stats[i].max);	
					}
				}
				if (!iwdli->haveData) fprintf(fp, "1970-1-1 0:0:0 0 0\n");
				// neerschrijven in de gnuplot script van data gemiddelde
				fprintf(fp, "e\n");
				prevt = 0;
				for (unsigned i = 0; i <  iwdli->statLen; i++)
				{
					if (iwdli->stats[i].nr)
					{	
						t = iwdli->from + i * interval;		
						if (prevt and t - prevt > iwdli->valid_time and t - prevt > (1.1 * interval))
							fprintf(fp, "\n");
						prevt = t;
						fprintt(fp, t + interval / 2);
						fprintf(fp, " %.8f\n", iwdli->stats[i].avg);	
					}
				}
				if (!iwdli->haveData) fprintf(fp, "1970-1-1 0:0:0 0 0\n");
			}
		}
		fclose(fp);
		
		DBG("gps written: %s\n", scriptName); 	
		// Klaar. start gnuplot.
		char systemCommand[1100];
		snprintf(systemCommand, 1100, "gnuplot \"%s\"", scriptName);
		system(systemCommand);
		

		// Weg met de gpl script file
		//remove(scriptName);
		setFileLife(scriptName, now() + 3600);

		// noem deze file naam in verband met autodelete
		setFileLife(imageName, now() + 3600);
		return imageUrl;}
	return "no URL";
	}

// Plot data in a cyclical way. inlist, tmin, tmax, x, y, title are just
// as for plotLines, interval is the cyclical interval.
// Coloring should follow a gradient. Older lines are lighter colored.
string plotcycle(list<in*> ins, double tmin, double tmax, unsigned int x, unsigned int y, string title, double cycle)
{
	DBG("plotcycle(list of %ld, %lf, %lf, %d, %d, %s, %lf", ins.size(), tmin, tmax, x, y, title.c_str(), cycle);
	char imageName[1025], scriptName[1025], imageUrl[1025];
	string names;
	bool haveKey(ins.size() > 1);
	double firstcycle = floor(tmin / cycle);
	double lastcycle = ceil(tmax / cycle);
	unsigned int cycles = lastcycle - firstcycle + 1;
		double bins = x * 0.7 * cycles;
	if (haveKey) bins = x * 0.4 * cycles;

	// build the filename(s)
	uint16_t pn = plotnr++; 
	snprintf(imageName, 1024, "http/graphs/%x.png", pn);
	snprintf(scriptName, 1024, "http/graphs/%x.gps", pn);
	snprintf(imageUrl, 1024, "/graphs/%x.png", pn);
	// Leave if file already exists.
	FILE* fp = fopen(imageName, "r");
	if (fp)
	{
		fclose(fp);
		return imageUrl;
	}
	string path = "http/";	
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	path += "graphs/";
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	fp = fopen(scriptName, "w");
	if (fp)
	{
		// Assign ins to y_axes.
		list<y_ax> y_axes;
		for(auto ini=ins.begin(); ini!=ins.end(); ini++)
		{
			string units;
			units = (*ini)->getUnits();
			bool foundyax(false);
			for(auto yi=y_axes.begin(); yi!=y_axes.end(); yi++)
				if (yi->units == units)
				{		
					inWithData iwd;
					iwd.inp = (*ini);
					iwd.stats.resize(bins);
					iwd.statLen = bins;
					iwd.valid_time = (*ini)->get_valid_time();
					iwd.from = tmin - iwd.valid_time;
					iwd.to = tmax + iwd.valid_time;
					(*ini)->getDataSummary(iwd.stats, iwd.statLen, iwd.from, iwd.to);
					yi->iwdl.push_back(iwd);
					foundyax = true;
				}
			if (not foundyax)
			{
				inWithData iwd;
				iwd.inp = (*ini);
				iwd.stats.resize(bins);
				iwd.statLen = bins;
				iwd.valid_time = (*ini)->get_valid_time();
				iwd.from = tmin - iwd.valid_time;
				iwd.to = tmax + iwd.valid_time;
				(*ini)->getDataSummary(iwd.stats, iwd.statLen, iwd.from, iwd.to);
				list<inWithData> iwdl;								
				iwdl.push_back(iwd);
				y_ax y;
				y.units = units;
				y.iwdl = iwdl;
				y.ymin =  -1;
				y.yrange = 2;
				y.ymax = 1;
				y.haveData = false;
				y_axes.push_back(y);
			}
		}
		// Now a list of y-axes exist. Per axis, calculate ymin, yrange and ymax.
		for(auto yi=y_axes.begin(); yi!=y_axes.end(); yi++)
		{
			for(auto iwdli=yi->iwdl.begin(); iwdli!=yi->iwdl.end(); iwdli++)
			{
				for(unsigned i = 0; i < iwdli->statLen; i++)
				{
					if (iwdli->stats[i].nr)
					{
						iwdli->haveData = true;
						if (yi->haveData == false)
						{
							yi->ymin = iwdli->stats[i].min;
							yi->ymax = iwdli->stats[i].max;
							yi->haveData = true;
						}
						else
						{
							if (yi->ymin > iwdli->stats[i].min)
								yi->ymin = iwdli->stats[i].min;
							if (yi->ymax < iwdli->stats[i].max)
								yi->ymax = iwdli->stats[i].max;
						}									
					}
				}
			}
			// ymin and ymax are now known, move them away from limits a little
			if (yi->ymax != yi->ymin)
			{ // range > 0 -> ymax != ymin. JCE, 16-7-13
				yi->yrange = yi->ymax - yi->ymin;
				yi->ymin -= 0.01 * yi->yrange;
				yi->ymax += 0.01 * yi->yrange;
			}
			else
			{	// ymin == ymax
				double margin(1);
				if (yi->ymax > 1000)
					margin = 0.01 * yi->ymax;
				yi->ymin -= margin;
				yi->ymax += margin;
			}
		}

		// So much for the multitude of axes. This only works for up to 2.
		bool haveY2(y_axes.size() > 1);
		y_ax y1, y2;
		y1 = *y_axes.begin();
		if (haveY2) y2 = (*(++y_axes.begin()));

		// Constructing the gnuplot file.			
		fprintf(fp, "# gnuplot script generated by tcFarmControl " GIT_SHORT_WORDHASH_WITH_MODIFIED " (" GIT_SHORT_HASH ")\n");
		fprintf(fp, "set terminal png size %u,%u truecolor\n", x, y); // truecolor option creates anti-aliasing. JCE< 5-7-13
		fprintf(fp, "set output \"%s\"\n", imageName);
		fprintf(fp, "set title \"%s\"\n", title.c_str());
		fprintf(fp, "set xdata time\n");
		fprintf(fp, "set timefmt \"");
		if ( cycle >= 24*60*60 ) fprintf(fp, "%%j_");
		if ( cycle >= 60*60 ) fprintf(fp, "%%H:");
		if ( cycle >= 60 ) fprintf(fp, "%%M:");
		fprintf(fp, "%%S\"\n");
		fprintf(fp, "set xrange [\"");
		fprintt(fp, 0, cycle);
		fprintf(fp, "\":\"");
		fprintt(fp, cycle, cycle);
		fprintf(fp, "\"]\n");		
		
		fprintf(fp, "set format x \"");
		if ( cycle > 24*60*60 ) fprintf(fp, "%%d ");
		if ( cycle > 60*60 ) fprintf(fp, "%%H:");
		if ( cycle > 60 ) fprintf(fp, "%%M:");
		fprintf(fp, "%%S\"\n");
		
		fprintf(fp, "set yrange [%.8f:%.8f]\n", y1.ymin, y1.ymax);
		fprintf(fp, "set ylabel \"%s\"\n", y1.units.c_str());
		if (haveY2)
		{
			fprintf(fp, "set y2range [%.8f:%.8f]\n", y2.ymin, y2.ymax);
			fprintf(fp, "set y2label \"%s\"\n", y2.units.c_str());
			fprintf(fp, "set y2tics\n");
		}
		if (haveKey)	
			fprintf(fp, "set key out vert right\n");
		else
			fprintf(fp, "set key off\n");
			
		fprintf(fp, "set grid\n");
		fprintf(fp, "set style fill transparent solid %f noborder\n", 0.10);
		string linecolor[] = {"blue", "red", "black", "green", "magenta", "brown", "spring-green", "cyan", "dark-orange", "violet", "sienna4", "steelblue", "dark-spring-green", "goldenrod"};
		unsigned int colornr(0);
		string plotline;
		bool first = true;
		for(auto yi=y_axes.begin(); yi!=y_axes.end(); yi++)
		{
			for(auto iwdli=yi->iwdl.begin(); iwdli!=yi->iwdl.end(); iwdli++)
			{
				// Deze moet getekend. Plot line
				if (first)
				{ 
					plotline += "plot\t";
					first = false;
				}
				else			
					plotline += ", \\\n\t";
				plotline += "\"-\" using 1:2:3";
				if (haveKey) plotline += " t '" + iwdli->inp->getName() + " [" + iwdli->inp->getUnits() + "]'";
				plotline += " with filledcu lc rgb \""+linecolor[colornr]+"\"";
				if (haveY2)
				{
					plotline += " axes ";
					if (iwdli->inp->getUnits() == y1.units)
						plotline += "x1y1";
					else
						plotline += "x1y2";
				}
				plotline += ", \\\n\t\"\" using 1:2";
				if (haveKey) plotline += " t '" + iwdli->inp->getName() + " [" + iwdli->inp->getUnits() + "]'";
				plotline += " with lines lc rgb \""+linecolor[colornr]+"\"";
				if (haveY2)
				{
					plotline += " axes ";
					if (iwdli->inp->getUnits() == y1.units)
						plotline += "x1y1";
					else
						plotline += "x1y2";
				}
				colornr++; if (colornr >= 14) colornr = 0;
			}
		}
		plotline += "\n";
		fprintf(fp, plotline.c_str());

		// Data content
		for(auto yi=y_axes.begin(); yi!=y_axes.end(); yi++)
		{
			for(auto iwdli=yi->iwdl.begin(); iwdli!=yi->iwdl.end(); iwdli++)
			{	
				double interval = (iwdli->to - iwdli->from) / bins;
				double t;
				
				if (yi != y_axes.begin() or iwdli != yi->iwdl.begin())
					fprintf(fp, "e\n");	
				double prevt = 0;
				
				// range		
				for (unsigned i = 0; i <  iwdli->statLen; i++)
				{
					if (iwdli->stats[i].nr)
					{			
						t = iwdli->from + i * interval;
						if (prevt and ((t - prevt > iwdli->valid_time and t - prevt > (1.1 * interval)) or ( floor((t + interval / 2) / cycle) != floor((prevt + interval / 2) / cycle) ) ))
							fprintf(fp, "\n");
						prevt = t;
						fprintt(fp, fmod(t + interval / 2, cycle), cycle);
						fprintf(fp, " %.8f %.8f\n", iwdli->stats[i].min, iwdli->stats[i].max);	
					}
				}
				if (!iwdli->haveData) fprintf(fp, "1970-1-1 0:0:0 0 0\n");
				
				// average
				fprintf(fp, "e\n");
				prevt = 0;
				for (unsigned i = 0; i <  iwdli->statLen; i++)
				{
					if (iwdli->stats[i].nr)
					{	
						t = iwdli->from + i * interval;		
						if (prevt and ((t - prevt > iwdli->valid_time and t - prevt > (1.1 * interval)) or ( floor((t + interval / 2) / cycle) != floor((prevt + interval / 2) / cycle) ) ))
							fprintf(fp, "\n");
						prevt = t;
						fprintt(fp, fmod(t + interval / 2, cycle), cycle);
						fprintf(fp, " %.8f\n", iwdli->stats[i].avg);	
					}
				}
				if (!iwdli->haveData) fprintf(fp, "1970-1-1 0:0:0 0 0\n");
			}
		}
		fclose(fp);
		
		DBG("gps written: %s\n", scriptName); 	
		// Klaar. start gnuplot.
		char systemCommand[1100];
		snprintf(systemCommand, 1100, "gnuplot \"%s\"", scriptName);
		system(systemCommand);
		

		// Weg met de gpl script file
		//remove(scriptName);
		setFileLife(scriptName, now() + 3600);

		// noem deze file naam in verband met autodelete
		setFileLife(imageName, now() + 3600);
		return imageUrl;}
	return "no URL";
	}

string plotLine(in* myIn, double tmin, double tmax, unsigned int x, unsigned int y){
	list<in*> inList;
	inList.push_back(myIn);
	return plotLines(inList, tmin, tmax, x, y, myIn->getName());
	}

string plotLines(in* i1, double tmin, double tmax, unsigned int x, unsigned int y, string title){
	list<in*> inList;
	inList.push_back(i1);
	return plotLines(inList, tmin, tmax, x, y, title);
	}

string plotLines(in* i1, in* i2, double tmin, double tmax, unsigned int x, unsigned int y, string title){
	list<in*> inList;
	inList.push_back(i1); inList.push_back(i2);
	return plotLines(inList, tmin, tmax, x, y, title);
	}

string plotLines(in* i1, in* i2, in* i3, double tmin, double tmax, unsigned int x, unsigned int y, string title){
	list<in*> inList;
	inList.push_back(i1); inList.push_back(i2); inList.push_back(i3);
	return plotLines(inList, tmin, tmax, x, y, title);
	}

string plotLines(in* i1, in* i2, in* i3, in* i4, double tmin, double tmax, unsigned int x, unsigned int y, string title){
	list<in*> inList;
	inList.push_back(i1); inList.push_back(i2); inList.push_back(i3); inList.push_back(i4);
	return plotLines(inList, tmin, tmax, x, y, title);
	}

string plotLines(in* i1, in* i2, in* i3, in* i4, in* i5, double tmin, double tmax, unsigned int x, unsigned int y, string title){
	list<in*> inList;
	inList.push_back(i1); inList.push_back(i2); inList.push_back(i3); inList.push_back(i4); inList.push_back(i5);
	return plotLines(inList, tmin, tmax, x, y, title);
	}

static int log_message(const struct mg_connection *conn, const char *message) {
	(void) conn;
	printf("%s\n", message);
	return 0;}

stringStore *noteStore;

double make_simple_header(struct mg_connection *conn, unsigned int autoRefreshTime = 0)
{
	
	double t = now();
	mg_printf(conn, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");
	if (autoRefreshTime)
		mg_printf(conn, "<meta http-equiv=\"refresh\" content=\"%u\">\n", autoRefreshTime);
	return t;
}

double make_header(struct mg_connection *conn, unsigned int autoRefreshTime = 0){
	double t = now();
	mg_printf(conn, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<html>\n");
	mg_printf(conn, "<head><title>tcFarmControl " GIT_SHORT_WORDHASH_WITH_MODIFIED "</title>\n");
	if (autoRefreshTime)
		mg_printf(conn, "<meta http-equiv=\"refresh\" content=\"%u\">\n", autoRefreshTime);
	mg_printf(conn, "</head><h1><a href=\"/\"><font color=\"#000000\"><span title=" GIT_SHORT_HASH ">tcFarmControl " GIT_SHORT_WORDHASH_WITH_MODIFIED "</span></font></a>\n");
	unsigned int outs = outsInManual();
	if (outs){
		mg_printf(conn, " - <a href=\"/manout\"><font color=\"#F00F00\">%u out", outs);
		if (outs > 1)
			mg_printf(conn, "s");
		mg_printf(conn, " in manual</font></a>\n");
		}
	mg_printf(conn, "</h1><hr>\n");
	return t;
	}

void make_footer(struct mg_connection *conn, double startTime){
	float generationTime = now() - startTime;
	float generationTimeMs = generationTime * 1000;
	mg_printf(conn, "<hr>\nPage generated in ");
	if (generationTime >= 1)
		mg_printf(conn, "%.3f seconds", generationTime);
	else
		mg_printf(conn, "%.2f ms", generationTimeMs);
	mg_printf(conn, "\n</html>\n");
	}

// Generate the html section for a note. The getter is a function that should return the content. The setter is a function that is supplied with the new content.
// returns if there has been found a new note. Suggested use: store note if it is new.
int make_note(struct mg_connection *conn, string &note){
	// note, met set en get
	char post_data[4096], newNote[4096];
	int post_data_len, rv(0);
	post_data_len = mg_read(conn, post_data, sizeof(post_data));
	if (post_data_len)
		if (mg_get_var(post_data, post_data_len, "note", newNote, sizeof(newNote)) > -1)
			{note = newNote;
			rv = 1;}
	mg_printf(conn, "<form method=\"POST\">Note<br><textarea rows=\"20\" name=\"note\" cols=\"60\" wrap=\"virtual\">%s</textarea><br><input type=\"submit\" value=\"Update\"/></form>", note.c_str()); 
	return rv;}

//void make_link(struct mg_connection *conn, string url, string text = ""){
//	if (text == "") text = url;
//	mg_printf(conn, "<a href=\"%s\">%s</a>",url, text);}

string make_link(string url, string text){
	if (text == "") text = url;
	return "<a href=\"" + url + "\">" + text + "</a>";}

string make_in_link(in* i, const string text)
{
	return make_link("/in/" + i->getDescriptor(), text);
}

string make_in_link(in* i)
{
	return make_link("/in/" + i->getDescriptor(), i->getName());
}

string make_webin_link(in* i, const string text)
{
	return make_link("/webin/" + i->getDescriptor(), text);
}


string make_out_link(out* o)
{
	return make_link("/out/" + o->getDescriptor(), o->getName());
}

string make_in_link_or_constant(in* i)
{
	if (!i)
		return "constant";
	return make_in_link(i);
}

string make_image(string url){
	return "<img src=\"" + url + "\">";}
string make_image_line(string url){
	return make_image(url) + "<br>";}

int make_in_graph_page(struct mg_connection *conn, in *i, double from, double to, uint16_t w=def_w, uint16_t h=def_h)
{
	mg_printf(conn, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<html>\n");
	string line;
	line = make_image_line(plotLine(i, from, to, w, h));
	mg_printf(conn, line.c_str());
	return 1;
}

int make_in_cycle_page(struct mg_connection *conn, in *i, double from, double to, float cycle, uint16_t w=def_w, uint16_t h=def_h)
{
	mg_printf(conn, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<html>\n");
	string line;
	list<in*> inlist({i});
	line = make_image_line(plotcycle(inlist, from, to, w, h, i->getName(), cycle));
	mg_printf(conn, line.c_str());
	return 1;
}

int make_in_remove_reply(struct mg_connection *conn, size_t removed)
{
	mg_printf(conn, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<html>\n");
	mg_printf(conn, "Removed %zu records.", removed);
	return 1;
}

int make_equation_section(struct mg_connection *conn, equation *eq)
{
	mg_printf(conn, "Equation: %s<br>\n", eq->get_expression());
	mg_printf(conn, "<table><tr><th>Variable</th><th>Value</th><th>Source</th></tr>\n");
	const var_in_double *vars = eq->get_vars();
	int nr_vars = eq->get_nr_vars();
	for (int i = 0; i < nr_vars; i++)
	{
		mg_printf(conn, "<tr><td>%s</td><td>%f</td><td>%s</td><tr>\n", vars[i].var, vars[i].d, make_in_link_or_constant(vars[i].i).c_str());
	}	
	mg_printf(conn, "</table>\n");
	return 1;
}

int make_in_page(struct mg_connection *conn, in *i)
{
	// headers
	double start = make_header(conn, 60);
	
	// (short) name
	mg_printf(conn, "<h2>In page: %s</h2>\n", i->getName().c_str());
	// (long) name
	mg_printf(conn, "short name: %s<br>\n", i->getDescriptor().c_str()); // also could have been inName, hm...
	
	// current state
	mg_printf(conn, "last measurement: %.*f %s<br>\n", i->getDecimals(), i->getValue(), i->getUnits().c_str());

	// The age of this particular measurement
	mg_printf(conn, "measurement age: %.3f seconds<br>\n", i->getAge());
	
	// The validity of this measurement
	if (i->isValid())
		mg_printf(conn, "this in is considered valid<br>\n");
	else
		mg_printf(conn, "this in is considered invalid<br>\n");

	in_equation *ie = dynamic_cast<in_equation*>(i);
	if (ie)
	{
		make_equation_section(conn, ie->eq);
	}

	string line;
	line = make_image_line(plotLine(i, now() - 3600, now(), def_w, def_h));
	mg_printf(conn, line.c_str());
	line = make_image_line(plotLine(i, now() - 24*3600, now(), def_w, def_h));
	mg_printf(conn, line.c_str());
	line = make_image_line(plotLine(i, now() - 7*24*3600, now(), def_w, def_h));
	mg_printf(conn, line.c_str());
	line = make_image_line(plotLine(i, now() - 4*7*24*3600, now(), def_w, def_h));
	mg_printf(conn, line.c_str());

	char from[30], to[30];
	double tnow = now();
	write_human_time(from, tnow-24*3600);
	write_human_time(to, tnow);
	line = make_link(string("/in/") + i->getDescriptor() + "/cycle/3600/" + from + "/" + to + "/2000/600", "cyclic graph last day per hour");
	mg_printf(conn, line.c_str());
	mg_printf(conn, "<br>\n");

	write_human_time(from, tnow-7*24*3600);
	line = make_link(string("/in/") + i->getDescriptor() + "/cycle/86400/" + from + "/" + to + "/2000/600", "cyclic graph last week per day");
	mg_printf(conn, line.c_str());
	mg_printf(conn, "<br>\n");

	write_human_time(from, tnow-4*7*24*3600);
	line = make_link(string("/in/") + i->getDescriptor() + "/cycle/86400/" + from + "/" + to + "/2000/600", "cyclic graph last month per day");
	mg_printf(conn, line.c_str());
	mg_printf(conn, "<br>\n");
	
	write_human_time(from, tnow-4*7*24*3600);
	line = make_link(string("/in/") + i->getDescriptor() + "/cycle/604800/" + from + "/" + to + "/2000/600", "cyclic graph last month per week");
	mg_printf(conn, line.c_str());
	mg_printf(conn, "<br>\n");	
	
	mg_printf(conn, "A section of log data can be viewed as a list. Append /table/ or /table_h/(from)/(to) to the url. The times can be unix timestamps or human readable date/time: dd-mm-yyyy hh:mm:ss.<br>");
	mg_printf(conn, "A graph of a section of log data can be generated. Append /graph/(from)/(to)[/w/h] to the url. From and to can be unix timestamps or human readable time/date.<br>");
	mg_printf(conn, "Data can be removed from the dataset (takes some time) with /remove_(time_from, time_to, time_from_to, value_over, value_under, value_between)/(number or human readable datetime[/number or human readable datetime]. Removes data including the given limit(s)<br>");

	// note, met set en get
	string note = i->getNote();
	if (make_note(conn, note))
		i->setNote(note);

	// footer
	make_footer(conn, start);

	return 1;
} 

int make_in_list_page(struct mg_connection *conn){
	double now = make_header(conn, 10);
	mg_printf(conn, "<h2>List of in's</h2>");
	map<string, in*>::iterator i;
	// Print them sorted on their long, or human readable names. JCE, 16-7-13
	map<string, in*> nameSortedMap;
	for(i = inmap.begin(); i != inmap.end(); i++)
		nameSortedMap[i->second->getName()] = i->second;
	#ifdef debug
		printf("\nin iterating starting...\n");
	#endif
	for(i = nameSortedMap.begin(); i != nameSortedMap.end(); i++){
		#ifdef debug
			printf("in iteration started, constructing link\n");
		#endif
		if (i->second->hidden)
			continue;
		string link = "/in/" + i->second->getDescriptor();
		#ifdef debug
			printf("The inmap entry is: %s, The link is: %s, constructing link html\n", i->first.c_str(), link.c_str());
		#endif
		string linkhtml = make_link(link, i->second->getName());
		#ifdef debug
			printf("The link html is: %s, constructing final html line and giving it to mg_printf()\n", linkhtml.c_str());
		#endif
		if (not i->second->isValid()) mg_printf(conn, "<span style=\"background-color:red\">");
		mg_printf(conn,	"%s: %.*f %s<br>", linkhtml.c_str(), i->second->getDecimals(), i->second->getValue(), i->second->getUnits().c_str());
		if (not i->second->isValid()) mg_printf(conn, "</span>");
		mg_printf(conn, "\n");
		#ifdef debug
			printf("Written the html link to mg_printf, iteration finished\n");
		#endif
		}
	#ifdef debug
		printf("in iterating completed\n");
	#endif
	make_footer(conn, now);
	return 1;}


int make_webin_page(struct mg_connection *conn, string webinName){
	map<string, webin*>:: iterator i;		// Addressing by index is no good idea. It creates the element if it does not exist.
	i = webinmap.find(webinName);		// Fixed, JCE, 5-3-14
	if (i == webinmap.end()) {return 0;}
	webin* myIn = i->second;
	//webin* myIn = webinmap[webinName];
	//if (myIn == 0) {return 0;}

	char post_data[4096], newNote[4096], setdata[1024];
	int post_data_len;
	post_data_len = mg_read(conn, post_data, sizeof(post_data));
	if (post_data_len){
		if (mg_get_var(post_data, post_data_len, "value", setdata, sizeof(setdata)) > -1)
			{
			float newVal(0);
			if (sscanf(setdata, "%f", &newVal) == 1)
				myIn->setValue(newVal);
			}
		if (mg_get_var(post_data, post_data_len, "note", newNote, sizeof(newNote)) > -1)
			myIn->setNote(newNote);
		}


	// headers
	double start = make_header(conn, 60);
	
	// (short) name
	mg_printf(conn, "<h2>Web in page: %s</h2>\n", myIn->getName().c_str());
	// (long) name
	mg_printf(conn, "short name: %s<br>\n", myIn->getDescriptor().c_str()); // also could have been inName, hm...
	
	// current state
	mg_printf(conn, "last input: %.*f %s<br>\n", myIn->getDecimals(), myIn->getValue(), myIn->getUnits().c_str());

	// The age of this particular measurement
	//float age = now() - myIn->getTime();
	mg_printf(conn, "input age: %.3f seconds<br>\n", myIn->getAge());
		
	mg_printf(conn, "<form method=\"POST\">Set to:<INPUT type=\"TEXT\" name=\"value\" value=\"%.*f\"> %s<br>\n", myIn->getDecimals(), myIn->getValue(), myIn->getUnits().c_str());
	mg_printf(conn, "<INPUT type=\"submit\" name=\"Submit\" value=\"Submit\"></form><br>\n");

	#ifdef debug
		double tdata, tscript, tplot, tsum;
		string line;
		//line = make_image_line(plotLine(myIn, now() - 3600 + 1, now() + 1, def_w, def_h, tdata, tscript, tplot));
		//mg_printf(conn, line.c_str());
		//tsum = tdata + tscript + tplot;
		mg_printf(conn, "Graph generation times: fetch data: %f s, generate script: %f s, run gnuplot: %f s<br>\n", tdata, tscript, tplot);
		line = make_image_line(plotLine(myIn, now() - 24*3600 + 1, now() + 1, def_w, def_h, tdata, tscript, tplot));
		mg_printf(conn, line.c_str());
		tsum += tdata + tscript + tplot;
		mg_printf(conn, "Graph generation times: fetch data: %f s, generate script: %f s, run gnuplot: %f s<br>\n", tdata, tscript, tplot);
		line = make_image_line(plotLine(myIn, now() - 7*24*3600 + 1, now() + 1, def_w, def_h, tdata, tscript, tplot));
		mg_printf(conn, line.c_str());
		tsum += tdata + tscript + tplot;
		mg_printf(conn, "Graph generation times: fetch data: %f s, generate script: %f s, run gnuplot: %f s<br>\n", tdata, tscript, tplot);
		line = make_image_line(plotLine(myIn, now() - 4*7*24*3600 + 1, now() + 1, def_w, def_h, tdata, tscript, tplot));
		mg_printf(conn, line.c_str());
		tsum += tdata + tscript + tplot;
		mg_printf(conn, "Graph generation times: fetch data: %f s, generate script: %f s, run gnuplot: %f s<br>\n", tdata, tscript, tplot);
		mg_printf(conn, "Graph total time: %f s<br>\n", tsum);
	#else
		string line = make_image_line(plotLine(myIn, now() - 3600 + 1, now() + 1, def_w, def_h));
		mg_printf(conn, line.c_str());
		line = make_image_line(plotLine(myIn, now() - 24*3600 + 1, now() + 1, def_w, def_h));
		mg_printf(conn, line.c_str());
		line = make_image_line(plotLine(myIn, now() - 7*24*3600 + 1, now() + 1, def_w, def_h));
		mg_printf(conn, line.c_str());
		line = make_image_line(plotLine(myIn, now() - 4*7*24*3600 + 1, now() + 1, def_w, def_h));
		mg_printf(conn, line.c_str());
	#endif

	// note, met set en get
	string note(myIn->getNote());
	make_note(conn, note);
	make_footer(conn, start);
	return 1;} 


int make_webin_list_page(struct mg_connection *conn){
	double now = make_header(conn, 10);
	mg_printf(conn, "<h2>List of webin's</h2>");
	map<string, webin*>::iterator i;
	// Print them sorted on their long, or human readable names. JCE, 16-7-13
	map<string, webin*> nameSortedMap;
	for(i = webinmap.begin(); i != webinmap.end(); i++)
		nameSortedMap[i->second->getName()] = i->second;
	#ifdef debug
		printf("\nin iterating starting...\n");
	#endif
	for(i = nameSortedMap.begin(); i != nameSortedMap.end(); i++){
		#ifdef debug
			printf("in iteration started, constructing link\n");
		#endif
		string link = "/webin/" + i->second->getDescriptor();
		#ifdef debug
			printf("The webinmap entry is: %s, The link is: %s, constructing link html\n", i->first.c_str(), link.c_str());
		#endif
		string linkhtml = make_link(link, i->second->getName());
		#ifdef debug
			printf("The link html is: %s, constructing final html line and giving it to mg_printf()\n", linkhtml.c_str());
		#endif
		if (not i->second->isValid()) mg_printf(conn, "<span style=\"background-color:red\">");
		mg_printf(conn,	"%s: %.*f %s<br>", linkhtml.c_str(), i->second->getDecimals(), i->second->getValue(), i->second->getUnits().c_str());
		if (not i->second->isValid()) mg_printf(conn, "</span>");
		mg_printf(conn, "\n");
		#ifdef debug
			printf("Written the html link to mg_printf, iteration finished\n");
		#endif
		}
	#ifdef debug
		printf("in iterating completed\n");
	#endif
	make_footer(conn, now);
	return 1;}

int make_out_page(struct mg_connection *conn, string outName){
	map<string, out*>:: iterator i;		// Addressing by index is no good idea. It creates the element if it does not exist.
	i = outmap.find(outName);		// Fixed, JCE, 5-3-14
	if (i == outmap.end()) {return 0;}
	out* myOut = i->second;
	//if (myOut == 0) {return 0;}
	// catch manual setpoint if present
	char post_data[4096], newNote[4096], setdata[1024];
	int post_data_len;
	post_data_len = mg_read(conn, post_data, sizeof(post_data));
	if (post_data_len){
		if (mg_get_var(post_data, post_data_len, "set", setdata, sizeof(setdata)) > -1)
			{
			if (strncmp(setdata, "auto", 4) == 0)
				{myOut->setMan(false);}
			float mansetp(0);
			if (sscanf(setdata, "%f", &mansetp) == 1)
				{
				myOut->setManOut(mansetp);
				myOut->setMan(true);
				}
			if (sscanf(setdata, "manual: %f", &mansetp) == 1)
				{
				myOut->setManOut(mansetp);
				myOut->setMan(true);
				}
			}
		if (mg_get_var(post_data, post_data_len, "note", newNote, sizeof(newNote)) > -1)
			myOut->setNote(newNote);
		}
	double start = make_header(conn, 3600);
	mg_printf(conn, "<h2>Out page: %s</h2>\n", myOut->getName().c_str());
	mg_printf(conn, "short name: %s<br>\n", myOut->getDescriptor().c_str()); // also could have been inName, hm...
	mg_printf(conn, "last measurement: %.*f %s<br>\n", myOut->getDecimals(), myOut->getValue(), myOut->getUnits().c_str());
	mg_printf(conn, "measurement age: %.3f seconds<br>\n", myOut->getAge());
	float max(myOut->getMax()), min(myOut->getMin()), step(myOut->getStep()), states((max-min)/step + 1);
	unsigned int dec(myOut->getDecimals());
	mg_printf(conn, "Physical output range: from %.*f in steps of %.*f to %.*f. This is %.0f states.<br>\n", dec, min, dec, step, dec, max, states);
	if (myOut->isValid())
		mg_printf(conn, "this out is considered valid.<br>\n");
	else
		mg_printf(conn, "this out is considered invalid.<br>\n");
	if (myOut->getMan())
		mg_printf(conn, "this out is in manual mode, controlled by this webpage.<br>\n");
	else
		mg_printf(conn, "this out is in auto mode, controlled by the program logic.<br>\n");
	if (myOut->getControl())
		mg_printf(conn, "this out is controlling the pyhsical interface or output.<br>\n");
	else
		mg_printf(conn, "this out is not controlling the physical interface or output. Can be auto mode without controlling logic.<br>\n");
	if (myOut->getControl())	
		mg_printf(conn, "<form method=\"POST\">Set output<br><INPUT type=\"submit\" name=\"set\", value=\"auto (%.*f)\"><br>", dec, myOut->getOut());
	else
		mg_printf(conn, "<form method=\"POST\">Set output<br><INPUT type=\"submit\" name=\"set\", value=\"auto (no data)\"><br>");

	if (states <= 5)
		for (int i = states - 1; i >= 0; i--)
			mg_printf(conn, "<INPUT type=\"submit\" name=\"set\" value=\"manual: %.*f\"><br>", dec, min + step * i);
	else
		mg_printf(conn, "<INPUT type=\"submit\" name=\"blah\" value=\"manual:\"><INPUT type=\"TEXT\" name=\"set\" value=\"%.*f\"<br>", dec, myOut->getManOut());
	mg_printf(conn, "</form>");

	// Equation section
	mg_printf(conn, "Equation: %s<br>\n", myOut->expression.c_str());
	mg_printf(conn, "<table><tr><th>Variable</th><th>Value</th><th>Source</th></tr>\n");
	for (int i = 0; i < myOut->nr_vars; i++)
	{
		mg_printf(conn, "<tr><td>%s</td><td>%f</td><td>%s</td><tr>\n", myOut->vars[i].var, myOut->vars[i].d, make_in_link_or_constant(myOut->vars[i].i).c_str());
	}	
	mg_printf(conn, "</table>\n");
	// End equation section

	string line = make_image_line(plotLine(myOut, now() - 299, now() + 1, def_w, def_h)); // function rounds time, losing the most recent record. Thus, to 1 sec in the future. JCE, 25-7-13
	mg_printf(conn, line.c_str());

	set<in*> in_s;		// Intermediate step with a set is because ins might occur multiple times in vars. JCE, 15-11-2020
	in_s.insert(myOut);
	for (int i = 0; i < myOut->nr_vars; i++)
		if (myOut->vars[i].i)
			in_s.insert(myOut->vars[i].i);
	list<in*> ins(in_s.begin(), in_s.end());

	line = make_image_line(plotLines(ins, now() - 300, now(), def_w, def_h, ""));
	mg_printf(conn, line.c_str());
	line = make_image_line(plotLines(ins, now() - 24*3600, now(), def_w, def_h, ""));
	mg_printf(conn, line.c_str());
	//line = make_image_line(plotLine(myOut, now() - 4*7*24*3600, now(), def_w, def_h));
	//mg_printf(conn, line.c_str());
	string note(myOut->getNote());
	make_note(conn, note);
	make_footer(conn, start);
	return 1;}


int make_out_list_page(struct mg_connection *conn){
	double now = make_header(conn, 10);
	mg_printf(conn, "<h2>List of out's</h2>An out is an in, with additional properties. Thus, all outs are also visible on the in list.<br>\n");
		
	map<string, out*>::iterator i;
	// Print them sorted on their long, or human readable names. JCE, 16-7-13
	map<string, out*> nameSortedMap;
	for(i = outmap.begin(); i != outmap.end(); i++)
		nameSortedMap[i->second->getName()] = i->second;
	for(i = nameSortedMap.begin(); i != nameSortedMap.end(); i++){
		string link = "/out/" + i->second->getDescriptor();
		string linkhtml = make_link(link, i->second->getName());
		if (not i->second->isValid()) mg_printf(conn, "<span style=\"background-color:red\">");
		mg_printf(conn,	"%s: %.*f %s<br>", linkhtml.c_str(), i->second->getDecimals(), i->second->getValue(), i->second->getUnits().c_str());
		if (not i->second->isValid()) mg_printf(conn, "</span>");
		mg_printf(conn, "\n");
		}
	make_footer(conn, now);
	return 1;}

int make_man_out_list_page(struct mg_connection *conn){
	map<string, out*>::iterator i;
	char post_data[1024], setdata[1024];
	int post_data_len;
	post_data_len = mg_read(conn, post_data, sizeof(post_data));
	if (post_data_len)
		if (mg_get_var(post_data, post_data_len, "allAuto", setdata, sizeof(setdata)) > -1)
			for (i = outmap.begin(); i!= outmap.end(); i++)
				if (i->second->getMan())
					i->second->setMan(false);
	double now = make_header(conn, 10);
	mg_printf(conn, "<h2>List of out's in manual mode</h2>\n");
	if (outsInManual())
		mg_printf(conn, "<form method=\"POST\"><INPUT type=\"submit\" name=\"allAuto\", value=\"Set all to auto\"></form>\n");
	// Print them sorted on their long, or human readable names. JCE, 16-7-13
	map<string, out*> nameSortedMap;
	for(i = outmap.begin(); i != outmap.end(); i++)
		if (i->second->getMan())
			nameSortedMap[i->second->getName()] = i->second;
	for(i = nameSortedMap.begin(); i != nameSortedMap.end(); i++){
		string link = "/out/" + i->second->getDescriptor();
		string linkhtml = make_link(link, i->second->getName());
		if (not i->second->isValid()) mg_printf(conn, "<span style=\"background-color:red\">");
		mg_printf(conn,	"%s: %.*f %s<br>", linkhtml.c_str(), i->second->getDecimals(), i->second->getValue(), i->second->getUnits().c_str());
		if (not i->second->isValid()) mg_printf(conn, "</span>");
		mg_printf(conn, "\n");
		}
	make_footer(conn, now);
	return 1;}

int make_logic_page(struct mg_connection *conn, string logicName){
	map<string, logic*>:: iterator i;		// Addressing by index is no good idea. It creates the element if it does not exist.
	i = logics.find(logicName);		// Fixed, JCE, 5-3-14
	if (i == logics.end()) {return 0;}
	logic* myLogic = i->second;

	char post_data[4096], newNote[4096];//, setdata[1024];
	int post_data_len;
	post_data_len = mg_read(conn, post_data, sizeof(post_data));
	if (post_data_len){
		if (mg_get_var(post_data, post_data_len, "note", newNote, sizeof(newNote)) > -1)
			myLogic->setNote(newNote);
		}

	double start = make_header(conn, 60);
	
	mg_printf(conn, "<h2>Logic page: %s</h2>\n", myLogic->getName().c_str());
	mg_printf(conn, "short name: %s<br>\n", myLogic->getDescriptor().c_str()); // also could have been inName, hm...

	//mg_printf(conn, "%s\n", myLogic->getPageHtml().c_str());
	myLogic->make_page(conn);

	// note, met set en get
	mg_printf(conn, "<br>\n");
	string note(myLogic->getNote());
	make_note(conn, note);
	make_footer(conn, start);
	return 1;} 

int make_logic_list_page(struct mg_connection *conn){
	double now = make_header(conn, 10);
	mg_printf(conn, "<h2>List of logics</h2>");
	map<string, logic*>::iterator i;
	// Print them sorted on their long, or human readable names. JCE, 16-7-13
	map<string, logic*> nameSortedMap;
	for(i = logics.begin(); i != logics.end(); i++)
		nameSortedMap[i->second->getName()] = i->second;
	for(i = nameSortedMap.begin(); i != nameSortedMap.end(); i++){
		string link = "/logic/" + i->second->getDescriptor();
		string linkhtml = make_link(link, i->second->getName());
		mg_printf(conn,	"%s<br>", linkhtml.c_str());
		mg_printf(conn, "\n");
		}
	make_footer(conn, now);
	return 1;}

int make_root_page(struct mg_connection *conn){
	double now = make_header(conn);
	mg_printf(conn, make_link("/in", "inputs").c_str());
	mg_printf(conn, "<br>\n");
	mg_printf(conn, make_link("/webin", "web inputs").c_str());
	mg_printf(conn, "<br>\n");
	mg_printf(conn, make_link("/out", "outputs").c_str());
	mg_printf(conn, "<br>\n");
	mg_printf(conn, make_link("/manout", "outputs in manual mode").c_str());
	mg_printf(conn, "<br>\n");
	mg_printf(conn, make_link("/logic", "logics").c_str());
	mg_printf(conn, "<br>\n");
	mg_printf(conn, make_link("/jos", "job scheduler").c_str());
	mg_printf(conn, "<br>\n");
	mg_printf(conn, "about, page with build information, numbers, defined-at-compile times etc<br>\n");
	mg_printf(conn, "Note the note. If a page auto refreshes, the note in progress is lost.<br>\n");
	string note = noteStore->getString();
	if (make_note(conn, note))
		noteStore->setString(note);
	make_footer(conn, now);
	return 1;}

int make_jos_page(struct mg_connection *conn)
{
	double now = make_header(conn);
	mg_printf(conn, "<h2>job scheduler readout</h2>\n");
	mg_printf(conn, "<pre><code>\n");
	#define SIZE 100000
	char buf[SIZE];
	jos_printn(pool, buf, SIZE);
	mg_printf(conn, buf);
	mg_printf(conn, "</code></pre>\n");
	make_footer(conn, now);
	#undef SIZE
	return 1;
}

int make_configured_page(struct mg_connection *conn, string url)
{
	// It was attempted to not have webgui_page depend on mongoose stuff
	// but you have to know parameters by name in order for mongoose to
	// cough up their content. So variables are given predictable names
	// 0, 1, 2, 3, ... 11, 12, ... 200, 201
	// Dont worry, this is not the only ugly thing relating to mongoose.
	// Like receive buffers of fixed length...
	#define SIZE 100000
	vector<string> options;
	int i = 0;
	string number_as_string;
	string value_as_string;
	char post_data[SIZE];
	char post_data_item[SIZE];
	int post_data_len = mg_read(conn, post_data, SIZE);
	bool scan = post_data_len > 0;
	
	double t;
	if (page_has_header(url))
		t = make_header(conn);
	else
		t = make_simple_header(conn); // Sorry, but this also provides
		// some html headers, that are required.
	
	while(scan)
	{
		number_as_string = to_string(i);
		if (mg_get_var(post_data, post_data_len, number_as_string.c_str(), post_data_item, SIZE) > 0)
			options.push_back(post_data_item);
		else
			scan = false;
		i++;
	}
	
	mg_printf(conn, build_page(url, options).c_str());
	
	if (page_has_footer(url))
		make_footer(conn, t);
	
	return 1;
} 

in *mongoose_requests;
pthread_mutex_t request_counter_mutex = PTHREAD_MUTEX_INITIALIZER;
static int begin_request(struct mg_connection *conn) 
{
	pthread_mutex_lock(&request_counter_mutex);
	mongoose_requests->setValue(mongoose_requests->getValue() + 1);
	pthread_mutex_unlock(&request_counter_mutex);
	const struct mg_request_info *ri = mg_get_request_info(conn);
	#ifdef debug_mg
		printf("Mongoose: start request %s\n", ri->uri);
	#endif

	if (!strcmp(ri->uri, "/in") or !strcmp(ri->uri, "/in/"))
		return make_in_list_page(conn);
	if (!strncmp(ri->uri, "/in/", 4))
	{
		in *i;
		const char *uri = ri->uri + 4;
		double from, to;
		const char *slash1 = strpbrk(uri, "/");
		const char *slash2 = slash1 ? strpbrk(slash1+1, "/") : NULL;
		const char *slash3 = slash2 ? strpbrk(slash2+1, "/") : NULL;
		const char *slash4 = slash3 ? strpbrk(slash3+1, "/") : NULL;
		const char *slash5 = slash4 ? strpbrk(slash4+1, "/") : NULL;

		if (slash1)
			i = get_in(string(uri, slash1 - uri)); // http://a.b.c.d/in/in-name/more_follows
		else
			i = get_in(uri); // http://a.b.c.d/in/in-name						
		if (!i)
			return 0;
		if (slash2)
		{
			// http://a.b.c.d/in/in-name/table/*end*
			if (slash3 && !strncmp(slash1, "/table/", 7) && read_human_time(slash2+1, &from) && read_human_time(slash3+1, &to))
			{
				mg_printf(conn, table_fromto(i, from, to).c_str());
				return 1;
			}
			// http://a.b.c.d/in/in-name/table_h/*end*
			if (slash3 && !strncmp(slash1, "/table_h/", 9) && read_human_time(slash2+1, &from) && read_human_time(slash3+1, &to))
			{
				mg_printf(conn, table_fromto_h(i, from, to).c_str());
				return 1;
			}
			// http://a.b.c.d/in/in-name/table/start/end
			if (slash3 && sscanf(slash1, "/table/%lf/%lf", &from, &to) == 2)
			{
				mg_printf(conn, table_fromto(i, from, to).c_str());
				return 1;
			}
			// http://a.b.c.d/in/in-name/table_h/start/end
			if (slash3 && sscanf(slash1, "/table_h/%lf/%lf", &from, &to) == 2)
			{
				mg_printf(conn, table_fromto_h(i, from, to).c_str());
				return 1;
			}
			
			// Specific graph pages
			uint16_t w, h;
			// http://a.b.c.d/in/in-name/graph/24-7-2001 18:00:33/31-7-2001 16:35:00/width/heigth
			if (slash4 && !strncmp(slash1, "/graph/", 7) && read_human_time(slash2+1, &from) && read_human_time(slash3+1, &to) && sscanf(slash4, "/%" SCNu16 "/%" SCNu16, &w, &h) == 2)
				return make_in_graph_page(conn, i, from, to, w, h);			
			// http://a.b.c.d/in/in-name/graph/24-7-2001 18:00:33/31-7-2001 16:35:00
			if (slash3 && !strncmp(slash1, "/graph/", 7) && read_human_time(slash2+1, &from) && read_human_time(slash3+1, &to))
				return make_in_graph_page(conn, i, from, to);
			// http://a.b.c.d/in/in-name/graph/from/to			
			if (sscanf(slash1, "/graph/%lf/%lf", &from, &to) == 2)
				return make_in_graph_page(conn, i, from, to);
					
			// Cyclic graph pages
			float cycle;
			// http://a.b.c.d/in/in-name/cycle/cycletime/24-7-2001 18:00:33/31-7-2001 16:35:00/width/heigth
			if (slash5 && !strncmp(slash1, "/cycle/", 7) && sscanf(slash2, "/%f", &cycle) == 1 && read_human_time(slash3+1, &from) && read_human_time(slash4+1, &to) && sscanf(slash5, "/%" SCNu16 "/%" SCNu16, &w, &h) == 2)
				return make_in_cycle_page(conn, i, from, to, cycle, w, h);			
			// http://a.b.c.d/in/in-name/cycle/cycletime/24-7-2001 18:00:33/31-7-2001 16:35:00
			if (slash4 && !strncmp(slash1, "/cycle/", 7) && sscanf(slash2, "/%f", &cycle) == 1 && read_human_time(slash3+1, &from) && read_human_time(slash4+1, &to))
				return make_in_cycle_page(conn, i, from, to, cycle);
			// http://a.b.c.d/in/in-name/cycle/cycletime/from/to			
			if (slash4 && sscanf(slash1, "/cycle/%f/%lf/%lf", &cycle, &from, &to) == 3)
				return make_in_cycle_page(conn, i, from, to, cycle);
						
			// Trim commands, human readable
			if (slash2 && !strncmp(slash1, "/remove_time_from/", 16) && read_human_time(slash2+1, &from))
				return make_in_remove_reply(conn, i->remove_time_from(from));			
			if (slash2 && !strncmp(slash1, "/remove_time_to/", 14) && read_human_time(slash2+1, &to))
				return make_in_remove_reply(conn, i->remove_time_to(to));			
			if (slash3 && !strncmp(slash1, "/remove_time_from_to/", 19) && read_human_time(slash2+1, &from) && read_human_time(slash3+1, &to))
				return make_in_remove_reply(conn, i->remove_time_from_to(from, to));			
			// Trim commands, timestamp
			if (sscanf(slash1, "/remove_time_from/%lf", &from) == 1)
				return make_in_remove_reply(conn, i->remove_time_from(from));			
			if (sscanf(slash1, "/remove_time_to/%lf", &to) == 1)
				return make_in_remove_reply(conn, i->remove_time_to(to));			
			if (sscanf(slash1, "/remove_time_from)to/%lf/%lf", &from, &to) == 2)
				return make_in_remove_reply(conn, i->remove_time_from_to(from, to));			
			if (sscanf(slash1, "/remove_value_over/%lf", &from) == 1)
				return make_in_remove_reply(conn, i->remove_value_from(from));			
			if (sscanf(slash1, "/remove_value_under/%lf", &to) == 1)
				return make_in_remove_reply(conn, i->remove_value_to(to));			
			if (sscanf(slash1, "/remove_value_between/%lf/%lf", &from, &to) == 2)
				return make_in_remove_reply(conn, i->remove_value_from_to(from, to));			
		}
		return make_in_page(conn, i);
	}

	if (!strcmp(ri->uri, "/webin") or !strcmp(ri->uri, "/webin/"))
		return make_webin_list_page(conn);
	if (!strncmp(ri->uri, "/webin/", 7)){
		char webinName[513];
		sscanf(ri->uri, "/webin/%512s", webinName);
		return make_webin_page(conn, webinName);}
	
	if (!strcmp(ri->uri, "/out") or !strcmp(ri->uri, "/out/"))
		return make_out_list_page(conn);
	if (!strncmp(ri->uri, "/out/", 5)){
		char outName[513];
		sscanf(ri->uri, "/out/%512s", outName);
		return make_out_page(conn, outName);}

	if (!strcmp(ri->uri, "/manout") or !strcmp(ri->uri, "/manout/"))
		return make_man_out_list_page(conn);
	if (!strncmp(ri->uri, "/manout/", 8)){
		char outName[513];
		sscanf(ri->uri, "/manout/%512s", outName);
		return make_out_page(conn, outName);}

	if (!strcmp(ri->uri, "/logic") or !strcmp(ri->uri, "/logic/"))
		return make_logic_list_page(conn);
	if (!strncmp(ri->uri, "/logic/", 7)){
		char logicName[513];
		sscanf(ri->uri, "/logic/%512s", logicName);
		return make_logic_page(conn, logicName);}

	if (!strcmp(ri->uri, "/jos")){
		return make_jos_page(conn);}

	if (page_exists(ri->uri))
		return make_configured_page(conn, string(ri->uri) );
		
	if (!strcmp(ri->uri, "/") or !strcmp(ri->uri, "/index.htm") or !strcmp(ri->uri, "/index.html")){
		return make_root_page(conn);}

	#ifdef debug_mg
		printf("Mongoose: finished request %s\n", ri->uri);
	#endif

	return 0;}

mg_context *ctx;
void webGuiStart(string port){
	noteStore = new stringStore(tcDataDir "note.txt");
	mongoose_requests = new in("mongoose_rq", "Mongoose requests", "");
	struct mg_callbacks callbacks;
	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.log_message = &log_message;
	callbacks.begin_request = &begin_request;
	static const char *options[] = {
		"listening_ports", port.c_str(),
		NULL } ;
	ctx = mg_start(&callbacks, NULL, options);//(const char **) options);
	#ifdef debug_mg
		printf("mongoose started on port(s) %s with web root [%s]\n", mg_get_option(ctx, "listening_ports"), mg_get_option(ctx, "document_root"));
	#endif // debug_mg
	}

void webGuiStop(){
	#ifdef debug_mg
		printf("stopping mongoose...\n");
	#endif //debug_mg
	mg_stop(ctx);
	deleteAllFiles();
	delete noteStore;
	delete mongoose_requests;
	#ifdef debug_mg
		printf("mongoose stopped\n");
	#endif // debug_mg
	}

// Configuration
void config_webgui(json_t *json)
{
	if (json_is_object(json))
	{
		json_t *def_w_j, *def_h_j, *page_j;
		def_w_j = json_object_get(json, "def_w");
		def_h_j = json_object_get(json, "def_h");
		page_j = json_object_get(json, "page");
		if (json_is_integer(def_w_j))
			def_w = json_integer_value(def_w_j);
		if (json_is_integer(def_h_j))
			def_h = json_integer_value(def_h_j);
		if (json_is_object(page_j))
			build_page_data_from_json(page_j);
	}
}

