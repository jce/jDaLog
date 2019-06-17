#ifndef HAVE_INTERFACE_HOST_H
#define HAVE_INTERFACE_HOST_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "stringStore.h"

using namespace std;


class interface_host : public interface{
	public:
		interface_host(const string, const string); // descr, name, ip-as-string
		~interface_host();
		void getIns();
		in *diskfree, *diskused, *diskusedp;
		//in *fanspd, *requests, *resets, *scanrate, *uptime, *latency;
		// program cpu ins
		in *cpuus, *cpuss, *cpcus, *cpcss, *cputs, *cputp;
		// host cpu ins
		in *hcpuuser, *hcpunice, *hcpusystem, *hcpuidle, *hcpuiowait, *hcpuirq, *hcpusoftirq, *hcputotal, *hcputotalnoiowait;
		//in *hcpuuserp, *hcpusystemp, *hcpuidlep, *hcpuiowaitp, *hcpuirqp, *hcpusoftirqp, *hcputotalp, *hcputotalnoiowaitp;
		in *hcpuuserp, *hcpunicep, *hcpusystemp, *hcpuidlep, *hcpuiowaitp, *hcpuirqp, *hcpusoftirqp, *hcputotalp, *hcputotalnoiowaitp;
		in *num_threads, *priority, *vsize, *nice, *VmRSS; // *rss;
                //in *idrss, *isrss, *minflt, *majflt, *nswap, *inblock, *oublock, *nvcsw, *nivcsw;
		in *runtime, *totalruntime;
		in *uptime;
		in *cpuTemperature, *cpuFrequency;
	private:
		double _prevT;
		string _exec(const char*);
	};

#endif // HAVE_INTERFACE_HOST_H
