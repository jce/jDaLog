#ifndef HAVE_WEBSERVER_H
#define HAVE_WEBSERVER_H

#include <microhttpd.h>
#include <pthread.h>

class webserver
{
	public:
		webserver(std::string name, uint16_t threads, uint16_t port);
		~webserver();
		void start();
		void stop();
		enum MHD_Result handle_request(MHD_Connection*, const char*, const char*, 
							 const std::map<std::string, std::string>);
		uint16_t def_w = 1000, def_h = 300;
	private:
		string name;
		uint16_t threads;
		uint16_t port;
		struct MHD_Daemon *daemon;
		in *requests;
		pthread_mutex_t request_counter_mutex = PTHREAD_MUTEX_INITIALIZER;
		std::string webroot = "http";
		std::string make_in_page(in*);
		std::string make_webin_page(std::string);
		std::string make_logic_page(std::string);
		std::string make_out_page(std::string);
};

struct webserver_ctx
{
	webserver *srv;	
};

typedef struct webserver_ctx webserver_ctx;

void deleteOldFiles();

std::string make_note(std::string &note);
std::string make_link(std::string url, std::string text );
std::string make_in_link(in* i, const std::string text);
std::string make_in_link(in* i);
std::string make_webin_link(in* i, const std::string text);
std::string make_webin_list_page();
std::string make_out_link(out* o);
std::string make_in_link_or_constant(in* i);
std::string make_image(std::string url);
std::string make_image_line(std::string url);
std::string make_image_line(in *i, double from, double to, uint16_t w, uint16_t h);
std::string make_image_line(list<in*> inlist, double from, double to, uint16_t w, uint16_t h);


#endif // HAVE_WEBSERVER_H
