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
	command = "nmap -sP -n 10.0.0.0/24 > /dev/null; arp -an | grep ";
	command.append(_macstr);
	string result;
	result = exec(command.c_str());
	//printf(result.c_str()); 
	
	//string powerPage = get_html_page(_ipstr, 2);
	double end = now();
	double t = (start + end) / 2;
	searchtime->setValue((end-start), t); 
	mac_present->setValue(result.find(_macstr) != string::npos, t);
}

void interface_macp::setOut(out*, float){
	}
