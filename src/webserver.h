#ifndef HAVE_WEBSERVER_H
#define HAVE_WEBSERVER_H

#include <microhttpd.h>
#include <pthread.h>

class webserver{
	public:
		webserver(std::string name, uint16_t threads, uint16_t port);
		~webserver();
		void start();
		void stop();
		enum MHD_Result handle_request(MHD_Connection*, const char*, const char*, 
							const char*, const char*, long unsigned int*, void**);
		void config_webgui(json_t*);
	private:
		string name;
		uint16_t threads;
		uint16_t port;
		struct MHD_Daemon *daemon;
		in *requests;
		pthread_mutex_t request_counter_mutex = PTHREAD_MUTEX_INITIALIZER;
		uint16_t def_w = 1000, def_h = 300, plotnr = 0;
		string webroot = "http";
	};

void config_webgui(json_t*);

#endif // HAVE_WEBSERVER_H
