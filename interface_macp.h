#ifndef HAVE_INTERFACE_MACP_H
#define HAVE_INTERFACE_MACP_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "out.h"
#include "stringStore.h"

// interface to detect the presence of a MAC address in the LAN. JCE, 27-1-2016

using namespace std;

class interface_macp : public interface{
	public:
		interface_macp(const string, const string, const string); // descr, name, url
		~interface_macp();
		void getIns();
		in *mac_present, *searchtime;
		void setOut(out*, float);
	private:
		const string _macstr;
	};

#endif // HAVE_INTERFACE_JCEM_H
