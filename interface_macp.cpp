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
#include <iostream>
#include <cstdio>
#include <memory>
#include <regex>

#define debug

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

interface_macp::interface_macp(const string d, const string n, const string macstr):interface(d, n), _macstr(macstr){

	searchtime = new in(getDescriptor() + "_st", getName() + " searchtime", "s", 3);
	mac_present = new in(getDescriptor() + "_mp", getName() + " mac present", "", 0);
}

interface_macp::~interface_macp()
{
	delete mac_present;
	delete searchtime;
}

void interface_macp::getIns()
{
	double start = now();
	string command;
	//command = "nmap -sP -n 10.0.0.0/24 > /dev/null; arp -an | grep ";
	//command.append(_macstr);
	command = "nmap -sP -n 10.0.0.0/24 > /dev/null; arp";
	string result;
	result = exec(command.c_str());
	
	double end = now();
	double t = (start + end) / 2;
	searchtime->setValue((end-start), t); 

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
		if (macs.count(match_str) != 1)
			macs[match_str] = new in("mac_" + match_str, "mac " + match_str, "", 0);
	}

	// Update the status of known addresses.
	for (auto mac = macs.begin(); mac != macs.end(); mac++)
		mac->second->setValue(result.find(mac->first) != string::npos, t);

	// Separately scan also for this one mac.
	mac_present->setValue(result.find(_macstr) != string::npos, t);
}

void interface_macp::setOut(out*, float){
	}
