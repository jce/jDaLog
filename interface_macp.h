#ifndef HAVE_INTERFACE_MACP_H
#define HAVE_INTERFACE_MACP_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "jansson.h"
#include "out.h"
#include "stringStore.h"
#include <map>

// interface to detect the presence of a MAC address in the LAN. JCE, 27-1-2016

class interface_macp : public interface{
	public:
		interface_macp(const std::string, const std::string, float, const std::string, bool, bool); // descr, name, interval, pingrange, hidden_ins, track_all
		void look_for_mac(const string&);
		void track_all_macs();
		~interface_macp();
		void getIns();
		in *searchtime;
		void setOut(out*, float);
		void add_mac(const char*, const char*, const char*);
	private:
		std::map<std::string, in*> macs;
		std::map<std::string, in*> macs_auto;
		const std::string pingrange = "";
		bool hidden_ins = false;
		bool _trackallmacs = false;
	};

void add_macs_from_array(interface_macp&, json_t*);

#endif // HAVE_INTERFACE_JCEM_H
