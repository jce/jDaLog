#ifndef HAVE_INTERFACE_MARIA_H
#define HAVE_INTERFACE_MARIA_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "out.h"
#include "stringStore.h"
//  sudo apt-get install libmariadbclient-dev
#include "mysql/mysql.h"
#include "pthread.h"

class interface_maria : public interface{
	public:
		interface_maria(const std::string, const std::string); // descr, name
		~interface_maria();
		void getIns();
		in *responseTime;
		void setOut(out*, float);

	private:
		const char* host = "localhost";
		const char* username = "tcFC";
		const char* password = "tcFC_pwd123!!_a5df";
		const char* database = "Farm";
		uint16_t port;		
		MYSQL *mysql;

		void connect();
		void reconnect();
		static void* thread_fnc_cc(void*);
		pthread_t thread;

	};

#endif // HAVE_INTERFACE_DARKSKY_H
