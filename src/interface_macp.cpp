#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_macp.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "out.h"
#include "webgui.h"

#include <iostream>
#include <cstdio>
#include <memory>
#include <regex>
#include <filesystem>	// C++17, directory iterator

#define DBG(...) { printf(__VA_ARGS__); printf("\n"); }
//#define DBG(...)

using namespace std;

// http://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c
string exec(const char* cmd) 
{
    shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != NULL)
            result += buffer;
    }
    return result;
}

interface_macp::interface_macp(const string d, const string n, float i, const string pr, bool h, bool t):interface(d, n, i), pingrange(pr), hidden_ins(h), _trackallmacs(t)
{
	searchtime = new in(getDescriptor() + "_st", getName() + " searchtime", "s", 3);
	searchtime->set_valid_time(interval * IN_VALIDTIME_SCAN_MULTIPLY);
	if (_trackallmacs)
	{
		found_new_mac = new in(getDescriptor() + "_fnm", getName() + " found new mac");
		found_new_mac->set_valid_time(interval * IN_VALIDTIME_SCAN_MULTIPLY);
		logic_mac = new logic_mac_c(getDescriptor(), getName(), found_new_mac, macs_auto);
		// Data is stored in #define tcDataDir + /in/ then in a dir.
		// Try and reconstruct any mac_ to in's 
		for( auto& p: filesystem::directory_iterator(tcDataDir "in/"))
			if (p.is_directory())
			{
				string stem = p.path().stem().string();
				size_t pos = stem.find(getDescriptor() + "_");
				if (pos != string::npos)
				{
					string mac(stem, pos+4);
					//printf(mac.c_str()); printf("\n");
					int x;
					if (sscanf(mac.c_str(), "%*2x:%*2x:%*2x:%*2x:%*2x:%2x", &x) == 1)
					{
						//printf("match\n");
						macs_auto[mac] = new in(getDescriptor() + "_" + mac, getName() + " " + mac, "", 0);
						macs_auto[mac]->hidden = h;
						macs_auto[mac]->set_valid_time(interval * IN_VALIDTIME_SCAN_MULTIPLY);
					}
				}
			}
	}
}

interface_macp::~interface_macp()
{
	delete searchtime;
	if (_trackallmacs)
	{
		delete found_new_mac;
		delete logic_mac;
	}
	for (auto mac = macs_auto.begin(); mac != macs_auto.end(); mac++)
		delete mac->second;
	for (auto mac = macs.begin(); mac != macs.end(); mac++)
		delete mac->second;
}

void interface_macp::getIns()
{
	double start = now();
	string command;
	//command = "nmap -sP -n 10.0.0.0/24 > /dev/null; arp -an | grep ";
	//command.append(_macstr);
	command = "nmap -sn -n " + pingrange + " 2>&1 > /dev/null; arp"; // Pingrange example 10.0.0.0/24
	string result;
	result = exec(command.c_str());
	
	double end = now();
	double t = (start + end) / 2;
	searchtime->setValue((end-start), t); 

	if (_trackallmacs)
	{
		// Find all mac addresses, add an entry in macs for new addresses.
		// Ooh i had one problem, then i tought to fix it with an regex. Now i have two problems...
		regex mac_regex("([0-9a-f]{2}:[0-9a-f]{2}:[0-9a-f]{2}:[0-9a-f]{2}:[0-9a-f]{2}:[0-9a-f]{2})");
		auto macs_begin = sregex_iterator(result.begin(), result.end(), mac_regex);
		auto macs_end = sregex_iterator();
		for (sregex_iterator i = macs_begin; i != macs_end; i++)
		{
			smatch match = *i;
			string match_str = match.str();
			//printf(match_str.c_str());
			
			// Create in's for new mac addresses
			if (macs_auto.count(match_str) != 1)
			{
				found_new_mac->setVal(1);
				macs_auto[match_str] = new in(getDescriptor() + "_" + match_str, getName()+ " " + match_str, "", 0);
				macs_auto[match_str] -> hidden = hidden_ins;
				macs_auto[match_str] -> set_valid_time(interval * IN_VALIDTIME_SCAN_MULTIPLY);
			}
			else
				found_new_mac->touch();
		}
	}

	// Update the status of known addresses.
	for (auto mac = macs_auto.begin(); mac != macs_auto.end(); mac++)
		mac->second->setValue(result.find(mac->first) != string::npos, t);

	for (auto mac = macs.begin(); mac != macs.end(); mac++)
		mac->second->setValue(result.find(mac->first) != string::npos, t);
}

void interface_macp::setOut(out*, float)
{
}

logic_mac_c::logic_mac_c(const string d, const string n, in* f, map<string, in*> &m): logic(d,n), found_new_mac(f), macs_auto(m)
{
	found_new_mac->register_callback_on_change(cc_logic_mac_c_on_found_new_mac_change, this);
}	

void logic_mac_c::on_found_new_mac_change()
{
	DBG("logic_mac_c::on_found_new_mac_change() called");
}

//int logic_mac_c::make_page(struct mg_connection *conn)
//{
	/*
	DBG("logic_mac_c::make_page(...) called");
	mg_printf(conn, "known mac list:\n");

	mg_printf(conn, "<table><tbody><tr><th>MAC</th><th>Status</th><th>Note</th></tr>\n");
	for (auto i = macs_auto.begin(); i != macs_auto.end(); i++)
	{	
		string onlinefield;
		if (i->second->getValue())
			onlinefield = "<td style=\"background-color:green;\">Online</td>";
		else
			onlinefield = "<td style=\"background-color:red;\">Offline</td>";
		string s = "<tr><td>" + make_in_link(i->second, i->first) + "</td>" + onlinefield + "<td>" + i->second->getNote().substr(0, 100) + "</td></tr>\n";
		mg_printf(conn, s.c_str());
		//DBG("%s", s.c_str());
	}
	mg_printf(conn, "</tbody></table>\n");
	*/
//	return 1;
//}

void interface_macp::add_mac(const char *macstr, const char *macdescr, const char *macname)
{
	if (!macstr)
	{
		printf("interface_macp %s (%s): No mac string given, mac not added\n", getDescriptor().c_str(), getName().c_str());
		return;
	}
	if (!macdescr)
		macdescr = macstr;
	if (!macname)
		macname = macdescr;
	macs[macstr] = new in(getDescriptor() + "_" + macdescr, getName()+ " " + macname, "", 0);
	macs[macstr] -> set_valid_time(interval * IN_VALIDTIME_SCAN_MULTIPLY);
}

