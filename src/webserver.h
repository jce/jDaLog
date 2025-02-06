#ifndef HAVE_WEBSERVER_H
#define HAVE_WEBSERVER_H

class webserver{
	public:
		webserver(std::string name, uint16_t threads, uint16_t port);
		~webserver();
		void start();
		void stop();
		enum MHD_Result handle_request(MHD_Connection*, const char*, const char*, 
							const char*, const char*, long unsigned int*, void**);
	private:
		string name;
		uint16_t threads;
		uint16_t port;
		struct MHD_Daemon *daemon;
		in *requests;
	};

#endif // HAVE_WEBSERVER_H
