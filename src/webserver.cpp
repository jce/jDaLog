
#include "equation.h"
#include "floatLog.h"
#include "git_header.h"
#include "in.h"
#include "in_equation.h"
#include "logic.h"
#include "main.h"
#include "out.h"
#include "stringStore.h"
#include "timefunc.h"
#include "webserver.h"
#include "webin.h"

#include <cstdlib> // system()
#include <fcntl.h>
#include <inttypes.h> // Macros like SCNu16
#include <list>
#include <math.h>
#include <set>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sys/stat.h>
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

struct connection_info 
{
    struct MHD_PostProcessor *pp;
    map<string, string> keyvalue;
};

enum MHD_Result post_iter(void *cls,
              enum MHD_ValueKind /*kind*/,
              const char *key,
              const char */*filename*/,
              const char */*content_type*/,
              const char */*transfer_encoding*/,
              const char *data,
              uint64_t /*off*/,
              size_t /*size*/)
{
    struct connection_info *ci = (struct connection_info *)cls;
    ci->keyvalue[key] = data;
    return MHD_YES;
}

// Default handler, https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html#microhttpd_002dcb
enum MHD_Result dh
	(
    	void *cls,
    	struct MHD_Connection *connection,
    	const char *url,
    	const char *method,
    	const char * /*version*/,
    	const char *upload_data,
    	long unsigned int *upload_data_size,
    	void **con_cls
	)
{
    struct connection_info *ci = (struct connection_info *) *con_cls;

    if (!ci)
    {
        ci = new struct connection_info;
        ci->pp = MHD_create_post_processor(connection, 1024, post_iter, ci);
        *con_cls = ci;
        return MHD_YES;
    }
    
    if (strcmp(method, "POST") == 0) 
    {
        if (*upload_data_size != 0) 
        {
            MHD_post_process(ci->pp, upload_data, *upload_data_size);
            *upload_data_size = 0;
            return MHD_YES;
        }
    }
    
	webserver *obj = (webserver*) cls;
	enum MHD_Result rv = obj->handle_request(connection, url, method, ci->keyvalue);

	for (auto i = ci->keyvalue.begin(); i != ci->keyvalue.end(); i++)
		DBG("%s: %s",  i->first.c_str(), i->second.c_str());

    delete ci;
    return rv;
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
		daemon = NULL;
		DBG("Stopped webserver %s", name.c_str());
	}
}

string make_root_page();

void deleteOldFiles()
{
	// https://stackoverflow.com/questions/249578/how-to-delete-files-older-than-x-hours
	system("find -L http/graphs -name '*.*' -mmin +3 -delete > /dev/null");
}

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

struct inWithData
{
	in* inp; 
	vector<flStat> stats; 
	unsigned statLen; 
	bool haveData;
	double valid_time;
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

// There is an issue where after pruning, the only items are changed-items. Making the 
// line just follow the average. Effectively the state in the bins before and/or after
// the change is forgotten. Try to fix this by introducing the left/right values.
void patch_stats_edge_bins(vector<flStat> &stats, unsigned bins)
{
	for (unsigned i = 0; i < bins-1; i++)
		if (stats[i+1].nr > 0 and stats[i].nr == 0)
		{
			stats[i].avg = stats[i+1].left;
			stats[i].min = stats[i].avg;
			stats[i].max = stats[i].avg;
			stats[i].nr = 1;
		}
	for (int i = bins-2; i >= 0; i--)
		if (stats[i+1].nr == 0 and stats[i].nr > 0)
		{
			stats[i+1].avg = stats[i].right;
			stats[i+1].min = stats[i+1].avg;
			stats[i+1].max = stats[i+1].avg;
			stats[i+1].nr = 1;
		}
}

// Plots a graph of given ins, between time tmin and tmax, with a width
// of x and heigth of y. The graph is stored in a specific location
// and the url to this location is returned.
void plotLines(list<in*> ins, double tmin, double tmax, unsigned int x, unsigned int y, string title, bool have_legend, string url)
{
	// Construct the image's filename, secondly get the units for x1 and x2, and number of plottable lines.
	string imageName, scriptName;
	list<in*>::iterator ini;
	list<inWithData>::iterator iwdli; 	// In with data list iterator
	bool haveKey = ins.size() > 1 and have_legend; // "Key" is Gnuplot language for "Legend", so it uses some graph-space.
	double bins = x * 0.7;				// Approximately one bin per pixel width of the graph.
	if (haveKey) bins = x * 0.4;

	imageName = "http/graphs/" + url;
	scriptName = imageName.substr(0, imageName.size() - 3) + "gps";
	
	string path = "http/";	
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	path += "graphs/";
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	FILE* fp = fopen(scriptName.c_str(), "w");
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
					patch_stats_edge_bins(iwd.stats, iwd.statLen);
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
				patch_stats_edge_bins(iwd.stats, iwd.statLen);
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

		// Deze implementatie is voor 2 y-assen. Nu moet gekeken worden of er 1 zijn of meer dan 1
		bool haveY2(y_axes.size() > 1);
		y_ax y1, y2;
		y1 = *y_axes.begin();
		if (haveY2) y2 = (*(++y_axes.begin()));

		// Het opbouwen van het gnuplot bestand			
		fprintf(fp, "# gnuplot script generated by jDaLog " GIT_SHORT_WORDHASH_WITH_MODIFIED " (" GIT_SHORT_HASH ")\n");
		fprintf(fp, "set terminal png size %u,%u truecolor\n", x, y); // truecolor option creates anti-aliasing. JCE< 5-7-13
		fprintf(fp, "set output \"%s\"\n", imageName.c_str());
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
		if      ( tmax-tmin > 364*24*60*60 	) fprintf(fp, "%%m-%%Y");
        else if ( tmax-tmin > 24*60*60 - 10	) fprintf(fp, "%%d-%%m");
		else if ( tmax-tmin > 60 			) fprintf(fp, "%%H:%%M");
		else if ( tmax-tmin <= 3600 - 10 	) fprintf(fp, "%%M:%%S");
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
                if (haveKey) plotline += " notitle ";
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
						if (prevt and t - prevt > (iwdli->valid_time * 1.1) and t - prevt > (1.1 * interval))
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
						if (prevt and t - prevt > (iwdli->valid_time * 1.1) and t - prevt > (1.1 * interval))
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
		
		// Klaar. start gnuplot.
		char systemCommand[1100];
		snprintf(systemCommand, 1100, "gnuplot \"%s\"", scriptName.c_str());
		system(systemCommand);
	}
}

// Plot data in a cyclical way. inlist, tmin, tmax, x, y, title are just
// as for plotLines, interval is the cyclical interval.
void plotcycle(list<in*> ins, double tmin, double tmax, unsigned int x, unsigned int y, string title, bool have_legend, string url, double cycle)
{
	DBG("plotcycle(list of %ld, %lf, %lf, %d, %d, %s, %lf", ins.size(), tmin, tmax, x, y, title.c_str(), cycle);
	string imageName, scriptName;
	bool haveKey(ins.size() > 1 and have_legend);
	double firstcycle = floor(tmin / cycle);
	double lastcycle = ceil(tmax / cycle);
	unsigned int cycles = lastcycle - firstcycle + 1;
		double bins = x * 0.7 * cycles;
	if (haveKey) bins = x * 0.4 * cycles;

	imageName = "http/graphs/" + url;
	scriptName = imageName.substr(0, imageName.size() - 3) + "gps";

	string path = "http/";	
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	path += "graphs/";
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	FILE* fp = fopen(scriptName.c_str(), "w");
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
					patch_stats_edge_bins(iwd.stats, iwd.statLen);
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
				patch_stats_edge_bins(iwd.stats, iwd.statLen);
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
		fprintf(fp, "# gnuplot script generated by jDaLog " GIT_SHORT_WORDHASH_WITH_MODIFIED " (" GIT_SHORT_HASH ")\n");
		fprintf(fp, "set terminal png size %u,%u truecolor\n", x, y); // truecolor option creates anti-aliasing. JCE< 5-7-13
		fprintf(fp, "set output \"%s\"\n", imageName.c_str());
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
						if (prevt and ((t - prevt > (iwdli->valid_time * 1.1) and t - prevt > (1.1 * interval)) or ( floor((t + interval / 2) / cycle) != floor((prevt + interval / 2) / cycle) ) ))
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
						if (prevt and ((t - prevt > (iwdli->valid_time * 1.1) and t - prevt > (1.1 * interval)) or ( floor((t + interval / 2) / cycle) != floor((prevt + interval / 2) / cycle) ) ))
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
		
		DBG("gps written: %s\n", scriptName.c_str()); 	
		// Klaar. start gnuplot.
		char systemCommand[1100];
		snprintf(systemCommand, 1100, "gnuplot \"%s\"", scriptName.c_str());
		system(systemCommand);
	}
}

vector<string> split(const char *s, char delimiter)
{
	vector<string> fields;
	const char *e = s;
	while(*e)
	{
		if (*e == delimiter) 
		{
			fields.push_back(string(s, e-s));
			s = e + 1;
		}
		e++;
	}
	fields.push_back(string(s));
    return fields;
}

void make_graph_from_url(const char *url)
{
	vector<string> fields = split(url, '+');

	// Find the in's. For convenience, we assume there are no
	// ins with the same name as keywords or parameters.
	list<in*> inlist;
	for (auto i = fields.begin(); i != fields.end(); i++)
	{
	    in *inp = get_in(*i);
	    if (inp)
	        inlist.push_back(inp);
	}

	// starttime, endtime, x and y dimensions
	double start, end;
	unsigned int x, y;
    if (not read_human_time(fields[fields.size() - 4].c_str(), &start))
	    sscanf(fields[fields.size()-4].c_str(), "%lf", &start);
    if (not read_human_time(fields[fields.size() - 3].c_str(), &end))
	    sscanf(fields[fields.size()-3].c_str(), "%lf", &end);
    sscanf(fields[fields.size()-2].c_str(), "%u", &x);
    sscanf(fields[fields.size()-1].c_str(), "%u", &y);

    // If the graph is a normal time graph	    
    if (fields.size() >= 6 and fields[0] == "/graph" and end > start)
        plotLines(inlist, start, end, x, y, "", true, url);

    // If the graph is cyclic
    if (fields.size() >= 7 and fields[0] == "/cyclic" and end > start)
    {
        if (fields[1] == "hour")
            plotcycle(inlist, start, end, x, y, "", true, url, 60*60);
        if (fields[1] == "day")
            plotcycle(inlist, start, end, x, y, "", true, url, 24*60*60);
        if (fields[1] == "week")
            plotcycle(inlist, start, end, x, y, "", true, url, 7*24*60*60);
    }
}

string make_simple_header(unsigned int autoRefreshTime = 0)
{
	string rv;
	rv += "<html>";//"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n";
	if (autoRefreshTime)
		rv += "<meta http-equiv=\"refresh\" content=\"" + to_string(autoRefreshTime) + "\">\n";
	return rv;
}

string make_header(unsigned int autoRefreshTime = 0)
{
	string rv;// = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<html>\n";
	rv += "<html><head><title>jDaLog " GIT_SHORT_WORDHASH_WITH_MODIFIED "</title>\n";
	if (autoRefreshTime)
		rv += "<meta http-equiv=\"refresh\" content=\"" + to_string(autoRefreshTime) + "\">\n";
	rv += "</head><h1><a href=\"/\"><font color=\"#000000\"><span title="  GIT_SHORT_HASH ">jDaLog " GIT_SHORT_WORDHASH_WITH_MODIFIED "</span></font></a>\n";
	unsigned int outs = outsInManual();
	if (outs)
	{
		rv += " - <a href=\"/manout\"><font color=\"#F00F00\">" + to_string(outs) + " out";
		if (outs > 1)	rv += "s";
		rv += " in manual</font></a>\n";
	}
	rv +="</h1><hr>\n";
	return rv;
}

string make_footer()
{	
	return "\n</html>\n";
}

string make_link(string url, string text )
{
	if (text == "") text = url;
	return "<a href=\"" + url + "\">" + text + "</a>";
}

string make_in_link(in* i, const string text) 	{ 	return make_link("/in/" + i->getDescriptor(), text);			}
string make_in_link(in* i)					 	{	return make_link("/in/" + i->getDescriptor(), i->getName());	}
string make_webin_link(in* i, const string text){	return make_link("/webin/" + i->getDescriptor(), text);			}
string make_out_link(out* o)					{	return make_link("/out/" + o->getDescriptor(), o->getName());	}
string make_in_link_or_constant(in* i)			{	if (!i)	return "constant";	return make_in_link(i);				}
string make_image(string url)					{	return "<img src=\"" + url + "\">";								}
string make_image_line(string url)				{	return make_image(url) + "<br>";								}
string make_image_line(in *i, double from, double to, uint16_t w, uint16_t h)
{
	string rv = "/graphs/graph+" + i->getDescriptor() + "+";
	rv += to_string(uint64_t (from)) + "+" + to_string(uint64_t (to));
	rv += "+" + to_string(w) + "+" + to_string(h) + ".png";
	return make_image_line(rv);
}
string make_image_line(list<in*> inlist, double from, double to, uint16_t w, uint16_t h)
{
	string rv = "/graphs/graph+"; 
	for (auto i = inlist.begin(); i != inlist.end(); i++)
		rv += (*i)->getDescriptor() + "+";
	rv += to_string(uint64_t (from)) + "+" + to_string(uint64_t (to));
	rv += "+" + to_string(w) + "+" + to_string(h) + ".png";
	return make_image_line(rv);
}

string make_in_remove_reply(size_t removed)
{
	return make_header() + "Removed " + to_string(removed) +  " records." + make_footer();
}

string make_equation_section(equation *eq)
{
	string rv = "Equation: " + string(eq->get_expression())  + " <br>\n";
	rv += "<table><tr><th>Variable</th><th>Value</th><th>Source</th></tr>\n";
	const var_in_double *vars = eq->get_vars();
	int nr_vars = eq->get_nr_vars();
	for (int i = 0; i < nr_vars; i++)
	{
		rv += "<tr><td>" + string(vars[i].var) + "</td><td>" + to_string(vars[i].d);
		rv += "</td><td>" + make_in_link_or_constant(vars[i].i) + "</td><tr>\n";
	}	
	rv += "</table>\n";
	return rv;
}

string webserver::make_in_page(in *i)
{
	string rv = make_header(60);
	rv += "<h2>In page: " + i->getName() + "</h2>\n";
	rv += "short name: " + i->getDescriptor() + "<br>\n";
	char buf[32];
	snprintf(buf, 32, "%.*f", i->getDecimals(), i->getValue());
	rv += string("last measurement: ") + buf + " " + i->getUnits() + "<br>\n";
	snprintf(buf, 32, "%.3f", i->getAge());
	rv += string("measurement age: ") + buf + " seconds<br>\n";
	// The validity of this measurement
	if (i->isValid())
		rv += "this in is valid<br>\n";
	else
		rv += "this in is invalid<br>\n";

	in_equation *ie = dynamic_cast<in_equation*>(i);
	if (ie)
	{
		rv += make_equation_section(ie->eq);
	}

	rv += make_image_line(i, now() - 3600, now(), def_w, def_h);
	rv += make_image_line(i, now() - 24*3600, now(), def_w, def_h);
	rv += make_image_line(i, now() - 7*243600, now(), def_w, def_h);
	rv += make_image_line(i, now() - 4*7*243600, now(), def_w, def_h);

	char from[32], to[32];
	double tnow = now();
	write_human_time(from, tnow-24*3600);
	write_human_time(to, tnow);
	rv += make_link(string("/graphs/cyclic+hour+") + i->getDescriptor() + "+" + from + "+" + to + "+2000+600", "cyclic graph last day per hour");
	rv += "<br>\n";

	write_human_time(from, tnow-7*24*3600);
	rv += make_link(string("/graphs/cyclic+day+") + i->getDescriptor() + "+" + from + "+" + to + "+2000+600", "cyclic graph last week per day");
	rv += "<br>\n";

	write_human_time(from, tnow-4*7*24*3600);
	rv += make_link(string("/graphs/cyclic+day+") + i->getDescriptor() + "+" + from + "+" + to + "+2000+600", "cyclic graph last month per day");
	rv += "<br>\n";
	
	write_human_time(from, tnow-4*7*24*3600);
	rv += make_link(string("/graphs/cyclic+week+") + i->getDescriptor() + "+" + from + "+" + to + "+2000+600", "cyclic graph last month per week");
	rv += "<br>\n";	
	
	rv += "A section of log data can be viewed as a list. Append /table/ or /table_h/(from)/(to) to the url. The times can be unix timestamps or human readable date/time: dd-mm-yyyy hh:mm:ss.<br>";
	rv += "A graph of a section of log data can be generated. Append /graph/(from)/(to)[/w/h] to the url. From and to can be unix timestamps or human readable time/date.<br>";
	rv += "Data can be removed from the dataset (takes some time) with /remove_(time_from, time_to, time_from_to, value_over, value_under, value_between)/(number or human readable datetime[/number or human readable datetime]. Removes data including the given limit(s)<br>";

	// footer
	rv += make_footer();
	return rv;
}

// Timestamps
std::string table_fromto(in *i, double from, double to) 
{
	string rv;
	map<double, float> data;
	i->getData(data, from, to);
    rv += "<table border=\"1\">\n";
	for (auto d = data.begin(); d != data.end(); d++)
	{	
		rv += "<tr><td>" + to_string(d->first) + "</td><td>" + to_string(d->second) + "</td></tr>\n";		
	}
    rv += "</table>";
	return rv;
}

// Formatted date/time	
std::string table_fromto_h(in *i, double from, double to) 
{
	string rv;
	char buf[50];
	map<double, float> data;
	i->getData(data, from, to);
    rv += "<table border=\"1\">\n";
	for (auto d = data.begin(); d != data.end(); d++)
	{	
        rv += "<tr><td>";
		write_human_time(buf, d->first);
		rv += buf;
		rv += "</td><td>" + to_string(d->second) + "</td></tr>\n";		
	}
    rv += "</table>";
	return rv;
}
 
string make_in_list_page()
{
	string rv = make_header(10);
	rv += "<h2>List of in's</h2>";
	map<string, in*>::iterator i;
	// Print them sorted on their long, or human readable names. JCE, 16-7-13
	map<string, in*> nameSortedMap;
	for(i = inmap.begin(); i != inmap.end(); i++)
		nameSortedMap[i->second->getName()] = i->second;
	for(i = nameSortedMap.begin(); i != nameSortedMap.end(); i++)
	{
		if (i->second->hidden)
			continue;
		string link = "/in/" + i->second->getDescriptor();
		string linkhtml = make_link(link, i->second->getName());
		if (not i->second->isValid()) 
			rv += "<span style=\"background-color:red\">";
		char buf[32];
		snprintf(buf, 32, "%.*f", i->second->getDecimals(), i->second->getValue());
		rv += linkhtml + ": " + buf + " " + i->second->getUnits();
		if (not i->second->isValid()) 
			rv += "</span>";
		rv += "<br>\n";	
	}
	rv += make_footer();
	return rv;
}

string webserver::make_webin_page(string webinName)
{
    webin *i = get_webin(webinName);
    if (i == NULL) 
        return "No webin with that name.";

    string rv = make_header(60);
	rv += "<h2>Web in page: " + i->getName() + "</h2>\n";
	rv += "short name: " + i->getDescriptor() + "<br>\n";
	char buf[32];
	snprintf(buf, 32, "%.*f", i->getDecimals(), i->getValue());
	rv += string("last input: ") + buf + " " + i->getUnits() + "<br>\n";
	snprintf(buf, 32, "%.3f", i->getAge());
	rv += string("input age: ") + buf + " seconds<br>\n";

	rv += make_image_line(i, now() - 3600, now(), def_w, def_h);
	rv += make_image_line(i, now() - 24*3600, now(), def_w, def_h);
	rv += make_image_line(i, now() - 7*243600, now(), def_w, def_h);
	rv += make_image_line(i, now() - 4*7*243600, now(), def_w, def_h);

	rv += make_footer();
	return rv;
} 

string make_webin_list_page()
{
	string rv = make_header(10);
	rv += "<h2>List of webin's</h2>";
	map<string, webin*> nameSortedMap;
	for(auto i = webinmap.begin(); i != webinmap.end(); i++)
		nameSortedMap[i->second->getName()] = i->second;
	for(auto i = nameSortedMap.begin(); i != nameSortedMap.end(); i++)
	{
		string link = "/webin/" + i->second->getDescriptor();
		string linkhtml = make_link(link, i->second->getName());
		if (not i->second->isValid()) 
		    rv += "<span style=\"background-color:red\">";
		char buf[32];
		snprintf(buf, 32, "%.*f", i->second->getDecimals(), i->second->getValue());
		rv += linkhtml + ": " + buf + " " + i->second->getUnits();
		if (not i->second->isValid()) 
			rv += "</span>";
		rv += "<br>\n";	
	}
	make_footer();
	return rv;
}

string webserver::make_out_page(string outName)
{
    string rv;
	out* o = get_out(outName);
	if (not o)
	    return "No out with that name.";
	
    /*
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
    */
	rv = make_header(10);
	rv += "<h2>Out page: " + o->getName() + "</h2>\n";
	rv += "short name: "+ o->getDescriptor() + "<br>\n";
    
    char buf[32];
	snprintf(buf, 32, "%.*f", o->getDecimals(), o->getValue());
	rv += string("last measurement: ") + buf + " " + o->getUnits() + "<br>\n";
	snprintf(buf, 32, "%.3f", o->getAge());
	rv += string("measurement age: ") + buf + " seconds<br>\n";
	// The validity of this measurement
	if (o->isValid())
		rv += "this out is valid<br>\n";
	else
		rv += "this out is invalid<br>\n";
    

	if (o->getMan())
		rv += "this out is in manual mode, controlled by this webpage.<br>\n";
	else
		rv += "this out is in auto mode, controlled by the program logic.<br>\n";
	if (o->getControl())
		rv += "this out is controlling the pyhsical interface or output.<br>\n";
	else
		rv += "this out is not controlling the physical interface or output. Can be auto mode without controlling logic.<br>\n";
	if (o->getControl())	
		rv += "<form method=\"POST\">Set output<br><input type=\"submit\" name=\"set\", value=\"auto\"><br>\n";
	else
		rv += "<form method=\"POST\">Set output<br>""<input type=\"submit\" name=\"set\", value=\"auto (no data)\"><br>\n";

	rv += "<INPUT type=\"submit\" name=\"set\" value=\"manual: 0\"><br>\n";
    rv += "<INPUT type=\"submit\" name=\"set\" value=\"manual: 1\"><br>\n";
    rv += string("<INPUT type=\"submit\" name=\"set\" value=\"manual:\">\n<INPUT type=\"TEXT\" name=\"setv\" value=\"") + to_string(o->getManOut()) + "\"<br>\n";
	rv += "</form>\n";

	in_equation *ie = dynamic_cast<in_equation*>(o);
	if (ie)
	{
		rv += make_equation_section(ie->eq);
	}

	//rv += make_image_line(o, now() - 3600, now(), def_w, def_h);
	//rv += make_image_line(o, now() - 24*3600, now(), def_w, def_h);
	//rv += make_image_line(o, now() - 7*243600, now(), def_w, def_h);
	//rv += make_image_line(o, now() - 4*7*243600, now(), def_w, def_h);

	char from[32], to[32];
	double tnow = now();
	write_human_time(from, tnow-24*3600);
	write_human_time(to, tnow);
	rv += make_link(string("/graphs/cyclic+hour+") + o->getDescriptor() + "+" + from + "+" + to + "+2000+600", "cyclic graph last day per hour");
	rv += "<br>\n";

	write_human_time(from, tnow-7*24*3600);
	rv += make_link(string("/graphs/cyclic+day+") + o->getDescriptor() + "+" + from + "+" + to + "+2000+600", "cyclic graph last week per day");
	rv += "<br>\n";

	write_human_time(from, tnow-4*7*24*3600);
	rv += make_link(string("/graphs/cyclic+day+") + o->getDescriptor() + "+" + from + "+" + to + "+2000+600", "cyclic graph last month per day");
	rv += "<br>\n";
	
	write_human_time(from, tnow-4*7*24*3600);
	rv += make_link(string("/graphs/cyclic+week+") + o->getDescriptor() + "+" + from + "+" + to + "+2000+600", "cyclic graph last month per week");
	rv += "<br>\n";	
	
	rv += "A section of log data can be viewed as a list. Append /table/ or /table_h/(from)/(to) to the url. The times can be unix timestamps or human readable date/time: dd-mm-yyyy hh:mm:ss.<br>";
	rv += "A graph of a section of log data can be generated. Append /graph/(from)/(to)[/w/h] to the url. From and to can be unix timestamps or human readable time/date.<br>";
	rv += "Data can be removed from the dataset (takes some time) with /remove_(time_from, time_to, time_from_to, value_over, value_under, value_between)/(number or human readable datetime[/number or human readable datetime]. Removes data including the given limit(s)<br>";

	rv += make_footer();
	return rv;
}


string make_out_list_page()
{
    string rv = make_header(10);
    rv += "<h2>List of out's</h2>An out is an in, with additional properties. Thus, all outs are also visible on the in list.<br>\n";
	map<string, out*>::iterator i;
	map<string, out*> nameSortedMap;
	for(i = outmap.begin(); i != outmap.end(); i++)
		nameSortedMap[i->second->getName()] = i->second;
	for(i = nameSortedMap.begin(); i != nameSortedMap.end(); i++)
    {
		string link = "/out/" + i->second->getDescriptor();
		string linkhtml = make_link(link, i->second->getName());
		if (not i->second->isValid()) 
            rv += "<span style=\"background-color:red\">";
        char buf[32];
        snprintf(buf, 32, "%.*f", i->second->getDecimals(), i->second->getValue());
        rv += linkhtml + ": " + buf + " " + i->second->getUnits();
        if (not i->second->isValid())
            rv += "</span>";
        rv += "<br>\n";
    }
    return rv;
}
/*
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
*/
string webserver::make_logic_page(string logicName)
{
	map<string, logic*>:: iterator i;
	i = logics.find(logicName);
	if (i == logics.end()) return "";
	logic* myLogic = i->second;

	string rv = make_header(60);
	rv += "<h2>Logic page: " + myLogic->getName()  + "</h2>\n";
	rv += "short name: " + myLogic->getDescriptor() + "<br>\n"; 

	//mg_printf(conn, "%s\n", myLogic->getPageHtml().c_str());
	webserver_ctx ctx;
	ctx.srv = this;
	rv += myLogic->make_page(&ctx);

	// note, met set en get
	rv += "<br>\n";
	//string note(myLogic->getNote());
	//make_note(conn, note);
	rv += make_footer();
	return rv;
} 

string make_logic_list_page()
{
	string rv = make_header() + "<h2>List of logics</h2>";
	map<string, logic*>::iterator i;
	// Print them sorted on their long, or human readable names. JCE, 16-7-13
	map<string, logic*> nameSortedMap;
	for(i = logics.begin(); i != logics.end(); i++)
		nameSortedMap[i->second->getName()] = i->second;
	for(i = nameSortedMap.begin(); i != nameSortedMap.end(); i++)
	{
		string link = "/logic/" + i->second->getDescriptor();
		string linkhtml = make_link(link, i->second->getName());
		rv += linkhtml + "<br>\n";;
	}
	rv += make_footer();
	return rv;
}

string make_root_page()
{
	string rv = make_header();
	rv += make_link("/in", "inputs") + "<br>";
	rv += make_link("/webin", "web inputs") + "<br>";
	rv += make_link("/out", "outputs") + "<br>";
	rv += make_link("/manout", "outputs in manual mode") + "<br>";
	rv += make_link("/logic", "logics") + "<br>";
	rv += make_footer();
	return rv;
}

enum MHD_Result webserver::handle_request
	(
    	struct MHD_Connection *connection,
    	const char * url,
    	const char * method,
    	const map<string, string> /*keyvalue*/
	)
{
	DBG("%s %s", method, url);

	string s;

	pthread_mutex_lock(&request_counter_mutex);
	requests->setValue(requests->getValue() + 1);
	pthread_mutex_unlock(&request_counter_mutex);

	if (!strcmp(url, "/in") or !strcmp(url, "/in/"))
		s = make_in_list_page();

	if (!strncmp(url, "/in/", 4))
	{
		in *i;
		const char *uri = url + 4;
		double from, to;
		const char *slash1 = strpbrk(uri, "/");
		const char *slash2 = slash1 ? strpbrk(slash1+1, "/") : NULL;
		const char *slash3 = slash2 ? strpbrk(slash2+1, "/") : NULL;
		//const char *slash4 = slash3 ? strpbrk(slash3+1, "/") : NULL;
		//const char *slash5 = slash4 ? strpbrk(slash4+1, "/") : NULL;

		if (slash1)
			i = get_in(string(uri, slash1 - uri)); // http://a.b.c.d/in/in-name/more_follows
		else
			i = get_in(uri); // http://a.b.c.d/in/in-name						
		
		if (i and slash2)
		{
		
			// http://a.b.c.d/in/in-name/table/ *end*
			if (slash3 && !strncmp(slash1, "/table/", 7) && read_human_time(slash2+1, &from) && read_human_time(slash3+1, &to))
				s = table_fromto(i, from, to);
			// http://a.b.c.d/in/in-name/table_h/ *end*
			if (slash3 && !strncmp(slash1, "/table_h/", 9) && read_human_time(slash2+1, &from) && read_human_time(slash3+1, &to))
				s = table_fromto_h(i, from, to);
			// http://a.b.c.d/in/in-name/table/start/end
			if (slash3 && sscanf(slash1, "/table/%lf/%lf", &from, &to) == 2)
				s = table_fromto(i, from, to);
			// http://a.b.c.d/in/in-name/table_h/start/end
			if (slash3 && sscanf(slash1, "/table_h/%lf/%lf", &from, &to) == 2)
				s = table_fromto_h(i, from, to);		
						
			// Trim commands, human readable
			if (slash2 && !strncmp(slash1, "/remove_time_from/", 16) && read_human_time(slash2+1, &from))
				s = make_in_remove_reply(i->remove_time_from(from));			
			if (slash2 && !strncmp(slash1, "/remove_time_to/", 14) && read_human_time(slash2+1, &to))
				s = make_in_remove_reply(i->remove_time_to(to));			
			if (slash3 && !strncmp(slash1, "/remove_time_from_to/", 19) && read_human_time(slash2+1, &from) && read_human_time(slash3+1, &to))
				s = make_in_remove_reply(i->remove_time_from_to(from, to));			
			// Trim commands, timestamp
			if (sscanf(slash1, "/remove_time_from/%lf", &from) == 1)
				s = make_in_remove_reply(i->remove_time_from(from));			
			if (sscanf(slash1, "/remove_time_to/%lf", &to) == 1)
				s = make_in_remove_reply(i->remove_time_to(to));			
			if (sscanf(slash1, "/remove_time_from)to/%lf/%lf", &from, &to) == 2)
				s = make_in_remove_reply(i->remove_time_from_to(from, to));			
			if (sscanf(slash1, "/remove_value_over/%lf", &from) == 1)
				s = make_in_remove_reply(i->remove_value_from(from));			
			if (sscanf(slash1, "/remove_value_under/%lf", &to) == 1)
				s = make_in_remove_reply(i->remove_value_to(to));			
			if (sscanf(slash1, "/remove_value_between/%lf/%lf", &from, &to) == 2)
				s = make_in_remove_reply(i->remove_value_from_to(from, to));			
		}
		else
			s = make_in_page(i);
	}
	
	if (!strcmp(url, "/webin") or !strcmp(url, "/webin/"))
		s = make_webin_list_page();
	if (!strncmp(url, "/webin/", 7))
        s = make_webin_page(url+7);

	if (!strcmp(url, "/out") or !strcmp(url, "/out/"))
		s = make_out_list_page();
	if (!strncmp(url, "/out/", 5))
        s = make_out_page(url+5);

/*
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
*/
	if (!strcmp(url, "/logic") or !strcmp(url, "/logic/"))
		s = make_logic_list_page();
	if (!strncmp(url, "/logic/", 7))
		s = make_logic_page(url + 7);

	// If any url looks in graph
	if (!strncmp(url, "/graphs/", 7))
		make_graph_from_url(url+7);

	if (!strcmp(url, "/") or !strcmp(url, "/index.htm") or !strcmp(url, "/index.html"))
		s = make_root_page();

	if (s != "")
	{
		// There is a response string. Answer the client.
    	struct MHD_Response *response;
    	MHD_Result ret;
		size_t size = s.size() * sizeof( char);
		char *buf = (char *) malloc( size);
		strncpy(buf, s.c_str(), size);
    	response = MHD_create_response_from_buffer(size, (void*) buf, MHD_RESPMEM_MUST_FREE);
		MHD_add_response_header (response, "Content-Type", "text/html");
    	ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    	MHD_destroy_response(response);
    	return ret;
	}
	// No response. See if a file exists at this url, else server error.

	string fap = webroot + url;	// File And Path
	int fd;
	struct stat sbuf;
	struct MHD_Response *response;
	enum MHD_Result ret;
	if ( ( -1==( fd = open ( fap.c_str(), O_RDONLY ) ) ) || ( 0 != fstat (fd, &sbuf)))
	{
		// No such file or error opening
		const char *errorstr = "<html><body>An internal server error has occurred!</body></html>";
		if (fd != -1)
			(void) close (fd);
		response = MHD_create_response_from_buffer(strlen(errorstr), (void *) errorstr, MHD_RESPMEM_PERSISTENT);
		if (NULL != response)
		{
			ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
			MHD_destroy_response(response);
			return ret;
		}
		else
			return MHD_NO;
	}
	response = MHD_create_response_from_fd(sbuf.st_size, fd);
	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);
		
	return ret;
}
