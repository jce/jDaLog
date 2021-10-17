#ifndef HAVE_K8055_H
#define HAVE_K8055_H

#include "filters.h"
#include "in.h"
#include "k8055.h"

//#include <pthread>
#include <stdio.h>
#include <string>
#include <map>

// Just a hacky interface to read one analog input (for now). Needs to be run as root... :(
// JCE, 16-10-2021

// To be able to contact the k8055 without being root:
// Add to /etc/udev/rules.d/99-com.rules
// SUBSYSTEM=="usb", ATTRS{idVendor}=="10cf", ATTRS{idProduct}=="5500", GROUP="gpio", MODE="0664"
// (assumed you are part of group gpio)
// then run "/etc/init.d/udev reload"

class interface_k8055 : public interface{
	public:
		interface_k8055(string, string); // descr, name, only accepts k8051 id 0
		~interface_k8055();
		void setOut(out*, float);
		void run();
	private:
		in *ai1; //, *ai2;
		out *ao1, *ao2;
		float outval1_new = 0, outval2_new = 0, outval1_written = 0, outval2_written = 0;
		blockfilter *bf;
		//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	};

void interface_k8055_from_json(const char*, const char*, json_t*);

#endif // HAVE_K8055_H
