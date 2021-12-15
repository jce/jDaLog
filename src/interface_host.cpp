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

//#define debug


using namespace std;

interface_host::interface_host(const string d, const string n, float i):interface(d, n, i), _prevT(0){
	add_disk("./", "host_disk", "Host disk");
	//diskfree    = new in("host_disk_free", "Host disk free", "byte");
	//diskused   = new in("host_disk_used", "Host disk used", "byte");
	//diskusedp  = new in("host_disk_usedp", "Host disk used p", "%", 7);
	//wd2diskfree    = new in("wd2_disk_free", "wd2 disk free", "byte");
	//wd2diskused   = new in("wd2_disk_used", "wd2 disk used", "byte");
	//wd2diskusedp  = new in("wd2_disk_usedp", "wd2 disk used p", "%", 7);
	//wd4diskfree    = new in("wd4_disk_free", "wd4 disk free", "byte");
	//wd4diskused   = new in("wd4_disk_used", "wd4 disk used", "byte");
	//wd4diskusedp  = new in("wd4_disk_usedp", "wd4 disk used p", "%", 7);
	//sdfree    = new in("sd_disk_free", "SD card free", "byte");
	//sdused   = new in("sd_disk_used", "SD card used", "byte");
	//sdusedp  = new in("sd_disk_usedp", "SD card used p", "%", 7);
	//ksfree    = new in("ks_disk_free", "Kingston free", "byte");
	//ksused   = new in("ks_disk_used", "Kingston used", "byte");
	//ksusedp  = new in("ks_disk_usedp", "Kingston used p", "%", 7);
	cpuus = new in("prog_utime", "Program cpu time user code", "s", 6);
	cpuss = new in("prog_stime", "Program cpu time system functions", "s", 6);
	cpcus = new in("prog_cutime", "Programs children cpu time user code", "s", 6);
	cpcss = new in("prog_cstime", "Programs children cpu time system functions", "s", 6);
	cputs = new in("prog_ttime", "Program cpu time total", "s", 6);
	cputp = new in("prog_ttimep", "Program cpu usage", "%", 6);


	num_threads = new in("prog_threads", "Program number of threads", "");
	priority = new in("prog_priority", "Program priority", "");
	vsize = new in("prog_vsize", "Program vsize", "bytes");
	//rsslim = new in("prog_rsslim", "Program rss limit", "byte");
	nice = new in("prog_nice", "Program nice value", "");
	//rss = new in("prog_rss", "Program rss", "pages");
	VmRSS = new in("prog_rss", "Program RSS", "bytes");
	/*maxrss = new in(getDescriptor() + "_maxrss", getName() + " prog physical memory allocated", "byte");
	ixrss = new in(getDescriptor() + "_ixrss", getName() + " prog code memory size", "byte");
	idrss = new in(getDescriptor() + "_idrss", getName() + " prog heap memory size", "byte");
	isrss = new in(getDescriptor() + "_isrss", getName() + " prog stack memory size", "byte");
	minflt = new in(getDescriptor() + "_minflt", getName() + " prog minor page faults: \"page reclaims\"", "");
	majflt = new in(getDescriptor() + "_majflt", getName() + " prog major page faults: \"swap in\"", "");
	nswap = new in(getDescriptor() + "_nswap", getName() + " prog page swaps", "");
	inblock = new in(getDescriptor() + "_inblock", getName() + " prog block input operations", "");
	oublock = new in(getDescriptor() + "_oublock", getName() + " prog block output operations", "");
	nvcsw = new in(getDescriptor() + "_nvcsw", getName() + " prog voluntary context switches", "");
	nivcsw = new in(getDescriptor() + "_nivcsw", getName() + " prog involuntary context switches", "");*/
	runtime = new in("prog_runtime", "Program runtime", "s");
	uptime = new in("host_uptime", "Host uptime", "s", 2);
	totalruntime = new in("prog_runtimetotal", "Program total runtime", "s");
		//in *hcpuuser, *hcpusystem, *hcpuidle, *hcpuiowait, *hcpuirq, *hcpusoftirq, *hcputotal, *hcputotalnoiowait;
	#ifdef debug
		printf("\nstart creating host cpu ins\n");
	#endif
	hcpuuser = new in("host_cpu_user", "Host cpu usermode", "s", 6);
	hcpunice = new in("host_cpu_nice", "Host cpu nicemode", "s", 6);
	hcpusystem = new in("host_cpu_system", "Host cpu systemmode", "s", 6);
	hcpuidle = new in("host_cpu_idle", "Host cpu idle", "s", 6);
	hcpuiowait = new in("host_cpu_iowait", "Host cpu iowait", "s", 6);
	hcpuirq = new in("host_cpu_irq", "Host cpu irq", "s", 6);
	hcpusoftirq = new in("host_cpu_softirq", "Host cpu softirq", "s", 6);
	hcputotal = new in("host_cpu_total", "Host cpu total used with iowait", "s", 6);
	hcputotalnoiowait = new in("host_cpu_total_noiowait", "Host cpu total used without iowait", "s", 6);
	#ifdef debug
		printf("\nended creating host cpu ins\n");
	#endif
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

void interface_host::getIns(){
	//double start = now();
	//double end = now();
	//double t = (start + end) / 2;
	//latency->setValue((end-start)/2*1000, t); // 2 requests, scale = ms

	#ifdef debug
		printf("\nstart interface_host::getIns()\n");
	#endif
	double thisT = now();
	static double startTime;
	if (!startTime) startTime = thisT;
	runtime->setValue(thisT - startTime, thisT);	
	
	// Data from /proc/blah/stat
	char procStatFile[101];
	snprintf(procStatFile, 100, "/proc/%u/stat", getpid());	
	#ifdef debug
		printf(procStatFile);
	#endif
	FILE *fp;
	bool allok = false;
	fp = fopen(procStatFile, "r");
	if (fp){
		int rv;
		int tty_nr;
		long unsigned minflt, cminflt, majflt, cmajflt, utime, stime, vsizein;
		long int rssin, cutime, cstime, num_threadsin, priorityin, nicein;

		rv = fscanf(fp, "%*d %*s %*c %*d %*d %*d %d %*d %*u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %*d %*u %lu %ld", &tty_nr, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime, &cutime, &cstime, &priorityin, &nicein, &num_threadsin, &vsizein, &rssin);
		#ifdef debug
			printf("decoded %i variables from proc stat file", rv);
			//printf("%d (%s) %c %d", pid, s, c, d);
			if (rv) printf("decoding proc stat file succesful");
			else printf("decoding proc stat file failed");
		#endif
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
			//rss->setValue(rssin);
			double totalCpuTime = (double) (utime + stime + cutime + cstime) / cps;
			if (_prevT) // Is deze init op 0?
				cputp->setValue((totalCpuTime - cputs->getValue()) / (thisT - _prevT) * 100);
			cputs->setValue(totalCpuTime);
			}
		}

	// Proc/stat
	#ifdef debug
		printf("\nstart extracting host cpu ins\n");
	#endif
	allok = false;
	fp = fopen("/proc/stat", "r");
	if (fp){
		long unsigned user, nice, system, idle, iowait, irq, softirq;

		int rv = fscanf(fp, "cpu %lu %lu %lu %lu %lu %lu %lu", &user, &nice, &system, &idle, &iowait, &irq, &softirq);
		fclose(fp); // JCE, 13-7-13, f*ck hier zat een open file leak...
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

	#ifdef debug
		printf("\nfinished extracting host cpu ins\n");
	#endif

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
	//di.free = new in(getDescriptor() + "_" + id + "f", getName() + " " + name + " free", "byte");
	//di.used = new in(getDescriptor() + "_" + id + "u", getName() + " " + name + " used", "byte");
	//di.usedp = new in(getDescriptor() + "_" + id + "p", getName() + " " + name + " used p", "%", 7);
	disks.push_back(di);
}
