#ifndef HAVE_INTERFACE_MACP_H
#define HAVE_INTERFACE_MACP_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "logic.h"
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
		void setOut(out*, float);
		void add_mac(const char*, const char*, const char*);
	private:
		logic *logic_mac = NULL;
		in *found_new_mac = NULL, *searchtime;
		std::map<std::string, in*> macs;			// Manually specified in config.
		std::map<std::string, in*> macs_auto;		// Automatically added to this list when detected.
		const std::string pingrange = "";
		bool hidden_ins = false;
		bool _trackallmacs = false;
	};

class logic_mac_c: public logic
{
	public:
		logic_mac_c(const std::string, const std::string, in*, std::map<std::string, in*>&);
		//int make_page(struct mg_connection*);
		//std::string make_page_string(nvm...
	private:
		in *found_new_mac;
		std::map<std::string, in*> &macs_auto;
		std::map<std::string, in*> macs_old;
		std::map<std::string, in*> macs_new;
		void on_found_new_mac_change();
		CC(logic_mac_c, on_found_new_mac_change);
};

void add_macs_from_array(interface_macp&, json_t*);
interface_macp* interface_macp_from_json(const json_t*);


#endif // HAVE_INTERFACE_JCEM_H
