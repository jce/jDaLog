#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_host.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "sys/statvfs.h" // statvfs()
#include "sys/resource.h" // getrusage
#include <sys/types.h> // getpid()
#include <unistd.h> // getpid() // sysconf
#include <stdint.h>
#include "unistd.h" // sysconf()
#include <unistd.h> // exec(), JCE, 3-9-2018
#include <iostream>
#include <cstdio>
#include <memory>

using namespace std;

interface_host::interface_host(const string d, const string n, float i):interface(d, n, i), _prevT(0){
	add_disk("./", "host_disk", "Host disk");
	cpuus = new in("prog_utime", "Program cpu time user code", "s", 6);
	cpuss = new in("prog_stime", "Program cpu time system functions", "s", 6);
	cpcus = new in("prog_cutime", "Programs children cpu time user code", "s", 6);
	cpcss = new in("prog_cstime", "Programs children cpu time system functions", "s", 6);
	cputs = new in("prog_ttime", "Program cpu time total", "s", 6);
	cputp = new in("prog_ttimep", "Program cpu usage", "%", 6);


	num_threads = new in("prog_threads", "Program number of threads", "");
	priority = new in("prog_priority", "Program priority", "");
	vsize = new in("prog_vsize", "Program vsize", "bytes");
	nice = new in("prog_nice", "Program nice value", "");
	VmRSS = new in("prog_rss", "Program RSS", "bytes");
	runtime = new in("prog_runtime", "Program runtime", "s");
	uptime = new in("host_uptime", "Host uptime", "s", 2);
	totalruntime = new in("prog_runtimetotal", "Program total runtime", "s");
	hcpuuser = new in("host_cpu_user", "Host cpu usermode", "s", 6);
	hcpunice = new in("host_cpu_nice", "Host cpu nicemode", "s", 6);
	hcpusystem = new in("host_cpu_system", "Host cpu systemmode", "s", 6);
	hcpuidle = new in("host_cpu_idle", "Host cpu idle", "s", 6);
	hcpuiowait = new in("host_cpu_iowait", "Host cpu iowait", "s", 6);
	hcpuirq = new in("host_cpu_irq", "Host cpu irq", "s", 6);
	hcpusoftirq = new in("host_cpu_softirq", "Host cpu softirq", "s", 6);
	hcputotal = new in("host_cpu_total", "Host cpu total used with iowait", "s", 6);
	hcputotalnoiowait = new in("host_cpu_total_noiowait", "Host cpu total used without iowait", "s", 6);
	hcpuuserp = new in("host_cpu_user_p", "Host cpu usermode %", "%", 6);
	hcpunicep = new in("host_cpu_nice_p", "Host cpu nicemode %", "%", 6);
	hcpusystemp = new in("host_cpu_system_p", "Host cpu systemmode %", "%", 6);
	hcpuidlep = new in("host_cpu_idle_p", "Host cpu idle %", "%", 6);
	hcpuiowaitp = new in("host_cpu_iowait_p", "Host cpu iowait %", "%", 6);
	hcpuirqp = new in("host_cpu_irq_p", "Host cpu irq %", "%", 6);
	hcpusoftirqp = new in("host_cpu_softirq_p", "Host cpu softirq %", "%", 6);
	hcputotalp = new in("host_cpu_total_p", "Host cpu total used with iowait %", "%", 6);
	hcputotalnoiowaitp = new in("host_cpu_total_noiowait_p", "Host cpu total used without iowait %", "%", 6);
		//in *hcpuuserp, *hcpusystemp, *hcpuidlep, *hcpuiowaitp, *hcpuirqp, *hcpusoftirqp, *hcputotalp, *hcputotalnoiowaitp;
#ifdef HAVE_RPI
	cpuTemperature = new in("host_cpu_temperature", "Host cpu temperature", "°C", 1); // JCE, 3-9-2018
	cpuFrequency = new in("host_cpu_frequency", "Host cpu frequency", "Hz", 0);
#endif // HAVE_RPI
	}

interface_host::~interface_host(){
	
#ifdef HAVE_RPI
	delete cpuFrequency;
	delete cpuTemperature; // JCE, 3-9-2018
#endif // HAVE_RPI

	for (auto i = disks.begin(); i != disks.end(); i++)
	{
		delete i->free;
		delete i->used;
		delete i->usedp;
	}

	delete cpuus;
	delete cpuss;
	delete cpcus;
	delete cpcss;
	delete cputs;
	delete cputp;

	delete num_threads;
	delete priority;
	delete vsize;
	delete nice;
	delete VmRSS;
	delete runtime; 
	delete uptime;
	delete totalruntime;
	
	delete hcpuuser;
	delete hcpunice;
	delete hcpusystem;
	delete hcpuidle;
	delete hcpuiowait;
	delete hcpuirq;
	delete hcpusoftirq;
	delete hcputotal;
	delete hcputotalnoiowait;
	delete hcpuuserp;
	delete hcpunicep;
	delete hcpusystemp;
	delete hcpuidlep;
	delete hcpuiowaitp;
	delete hcpuirqp;
	delete hcpusoftirqp;
	delete hcputotalp;
	delete hcputotalnoiowaitp;
	}

// Secret hidden helper function
// No longer hidden

// http://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c
string system_exec(const char* cmd) {
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

void interface_host::getIns()
{
	double thisT = now();
	static double startTime;
	if (!startTime) startTime = thisT;
	runtime->setValue(thisT - startTime, thisT);	
	
	// Data from /proc/blah/stat
	char procStatFile[101];
	snprintf(procStatFile, 100, "/proc/%u/stat", getpid());	
	FILE *fp;
	bool allok = false;
	fp = fopen(procStatFile, "r");
	if (fp){
		int rv;
		int tty_nr;
		long unsigned minflt, cminflt, majflt, cmajflt, utime, stime, vsizein;
		long int rssin, cutime, cstime, num_threadsin, priorityin, nicein;

		rv = fscanf(fp, "%*d %*s %*c %*d %*d %*d %d %*d %*u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %*d %*u %lu %ld", &tty_nr, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime, &cutime, &cstime, &priorityin, &nicein, &num_threadsin, &vsizein, &rssin);
		fclose(fp);
		allok = rv == 14;

		if (allok){
			int cps = sysconf(_SC_CLK_TCK);
			cpuus->setValue((double) utime / cps, thisT);
			cpuss->setValue((double) stime / cps, thisT);
			cpcus->setValue((double) cutime / cps, thisT);
			cpcss->setValue((double) cstime / cps, thisT);
			num_threads->setValue(num_threadsin);
			priority->setValue(priorityin);
			vsize->setValue(vsizein);
			nice->setValue(nicein);
			double totalCpuTime = (double) (utime + stime + cutime + cstime) / cps;
			if (_prevT)
				cputp->setValue((totalCpuTime - cputs->getValue()) / (thisT - _prevT) * 100);
			cputs->setValue(totalCpuTime);
			}
		}

	// Proc/stat
	allok = false;
	fp = fopen("/proc/stat", "r");
	if (fp){
		long unsigned user, nice, system, idle, iowait, irq, softirq;

		int rv = fscanf(fp, "cpu %lu %lu %lu %lu %lu %lu %lu", &user, &nice, &system, &idle, &iowait, &irq, &softirq);
		fclose(fp);
		allok = rv == 7;

		if (allok){
			int cps = sysconf(_SC_CLK_TCK);
			float userf = (float) user / cps;
			float nicef = (float) nice / cps;
			float systemf = (float) system / cps;
			float idlef = (float) idle / cps;
			float iowaitf = (float) iowait / cps;
			float irqf = (float) irq / cps;
			float softirqf = (float) softirq / cps;
			double dt = thisT - _prevT;
			if (_prevT){
				hcpuuserp->setValue((userf - hcpuuser->getValue()) / dt * 100, thisT);
				hcpunicep->setValue((nicef - hcpunice->getValue()) / dt * 100, thisT);
				hcpusystemp->setValue((systemf - hcpusystem->getValue()) / dt * 100, thisT);
				hcpuidlep->setValue((idlef - hcpuidle->getValue()) / dt * 100, thisT);
				hcpuiowaitp->setValue((iowaitf - hcpuiowait->getValue()) / dt * 100, thisT);
				hcpuirqp->setValue((irqf - hcpuirq->getValue()) / dt * 100, thisT);
				hcpusoftirqp->setValue((softirqf - hcpusoftirq->getValue()) / dt * 100, thisT);
				hcputotalp->setValue((userf + nicef + systemf + iowaitf + irqf + softirqf - hcputotal->getValue()) / dt * 100, thisT);
				hcputotalnoiowaitp->setValue((userf + nicef + systemf + irqf + softirqf - hcputotalnoiowait->getValue()) / dt * 100, thisT);
				}
			hcpuuser->setValue((float)user / cps, thisT);
			hcpunice->setValue((float)nice / cps, thisT);
			hcpusystem->setValue((float)system / cps, thisT);
			hcpuidle->setValue((float)idle / cps, thisT);
			hcpuiowait->setValue((float)iowait / cps, thisT);
			hcpuirq->setValue((float)irq / cps, thisT);
			hcpusoftirq->setValue((float)softirq / cps, thisT);
			hcputotal->setValue((float)(user+system+nice+iowait+irq+softirq) / cps, thisT);
			hcputotalnoiowait->setValue((float)(user+system+nice+irq+softirq) / cps, thisT);
			}
		}

	// Uptime, from /proc/uptime	
	allok = false;
	fp = fopen("/proc/uptime", "r");
	if (fp){
		float uptimef;
		int rv = fscanf(fp, "%f", &uptimef);
		allok = rv == 1;
		fclose(fp);
		if (allok)
			uptime->setValue(uptimef);
		}

	// Memory, from /proc/blah/statm
	char statmFile[101];
	snprintf(statmFile, 100, "/proc/%u/statm", getpid());	
	allok = false;
	fp = fopen(statmFile, "r");
	if (fp){
		float VmRSSIn;
		int rv = fscanf(fp, "%*u %f", &VmRSSIn);
		allok = rv == 1;
		fclose(fp);
		if (allok)
			VmRSS->setValue(VmRSSIn * sysconf(_SC_PAGESIZE));
		}

	// File system status, from library
	struct statvfs s;
	double totalBytes, freeBytes;
	for (auto i = disks.begin(); i != disks.end(); i++)
	{
		if (statvfs(i->path.c_str(), &s) == 0)
		{
			totalBytes = (double) s.f_frsize * s.f_blocks;
			freeBytes = (double) s.f_frsize * s.f_bfree;
			i->free->setValue(freeBytes, thisT);
			i->used->setValue(totalBytes - freeBytes, thisT);
			i->usedp->setValue( ((double) s.f_blocks - s.f_bfree) / s.f_blocks * 100, thisT);
		}
	}
	if (_prevT){
		double dt = thisT - _prevT;
		totalruntime->setValue(totalruntime->getValue() + dt);}

#ifdef HAVE_RPI
	// Raspberry pi 3 has a CPU temperature readout.
	// JCE, 3-9-2018
	string command, result;
	bool commandExecutedOK;
	//command = "sudo vcgencmd measure_temp";
	command = "vcgencmd measure_temp";
	result = system_exec(command.c_str());
	commandExecutedOK = (result.find("temp=") != string::npos);
	if (commandExecutedOK) cpuTemperature->setValue( stod(result.substr(5, 4)), thisT);

	command = "vcgencmd measure_clock arm";
	// Answer should be: frequency(45)=600000000 Note for Pi4 45 -> 48
	result = system_exec(command.c_str());
	commandExecutedOK = (result.find("frequency(48)=") != string::npos);
	if (commandExecutedOK) cpuFrequency->setValue( stod(result.substr(14)), thisT);
#endif // HAVE_RPI

	_prevT = thisT;
	}

void interface_host::add_disk(string path, string id, string name)
{
	disk_ins di;
	di.path = path;
	di.free = new in(id + "_free", name + " free", "byte");
	di.used = new in(id + "_used", name + " used", "byte");
	di.usedp = new in(id + "_usedp", name + " used p", "%", 7);
	disks.push_back(di);
}

interface_host* interface_host_from_json(const json_t *json )
{
	#define JSTR(_X_)	json_string_value(json_object_get(json, #_X_))
	#define JNR(_X_)	json_number_value(json_object_get(json, #_X_))
	#define JIN(_X_)	json_is_number(json_object_get(json, #_X_))
		
	const char *id = JSTR(id);
	const char *name = JSTR(name);
	if (not name)
		name = id;
	float scan = JNR(scan);		
	if (not (id and name and JIN(scan)))
	{
		printf("could not build interface_host(%s, %s, %f)\n", id, name, scan);
		return NULL;
	}
	interface_host *h = new interface_host(id, name, scan);
	const char* path;
	json_t *disk_j;
	json_object_foreach(json_object_get(json, "disks"), path, disk_j)
	{
		const char *did = json_string_value(json_object_get(disk_j, "id"));
		const char *dname = json_string_value(json_object_get(disk_j, "name"));
		
		h->add_disk(path, did, h->getName() + "_" + dname);
	}
	return h;
}

