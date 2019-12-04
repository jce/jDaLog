#ifndef HAVE_INTERFACE_MACP_H
#define HAVE_INTERFACE_MACP_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "out.h"
#include "stringStore.h"
#include <map>

// interface to detect the presence of a MAC address in the LAN. JCE, 27-1-2016

class interface_macp : public interface{
	public:
		interface_macp(const std::string, const std::string, const std::string); // descr, name, url
		void look_for_mac(const string&);
		void track_all_macs();
		~interface_macp();
		void getIns();
		in *searchtime, *mac_present;
		std::map<std::string, in*> macs;
		void setOut(out*, float);
	private:
		const std::string _macstr;
		bool _trackallmacs = false;
	};

#endif // HAVE_INTERFACE_JCEM_H
