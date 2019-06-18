#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_maria.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "out.h"
#include "unistd.h" // sleep

using namespace std;

interface_maria::interface_maria(const string d, const string n):interface(d, n)
{
	responseTime = new in(getDescriptor() + "_rt", getName() + " response time", "ms", 3);
	mysql_library_init(0, NULL, NULL);
	//mysql = mysql_init(NULL);
	pthread_create(&thread, NULL, &thread_fnc_cc, (void*) this);
}

interface_maria::~interface_maria(){
	pthread_cancel(thread);
	pthread_join(thread, 0);
	//mysql_close(mysql);
	//mysql_library_end();
	delete responseTime;
	}

void interface_maria::connect()
{
	printf("entering connect()\n");fflush(stdout);
	mysql = mysql_init(NULL);
	mysql_real_connect(mysql, host, username, password, database, port, NULL, 0);
	my_bool reconnect = 1;
	mysql_options(mysql, MYSQL_OPT_RECONNECT, &reconnect);
	printf("leaving connect()\n");fflush(stdout);
}

void interface_maria::reconnect()
{
	printf("entering reconnect()\n");fflush(stdout);
	mysql_close(mysql);
	connect();
	printf("leaving reconnect()\n");fflush(stdout);
}

void* interface_maria::thread_fnc_cc(void* imvp)
{
//	printf("%i", imvp);
	interface_maria* im = (interface_maria*) imvp;
	
//	mysql_library_init(0, NULL, NULL);
//	im->mysql = mysql_init(NULL);
	im->connect();
	time_t utctime;
	for(;;)
	{
		utctime = time(0);
		tm date_tm;
		localtime_r(&utctime, &date_tm);
		date_tm.tm_sec = 0;
		date_tm.tm_min = 0;
		date_tm.tm_hour = 0;
		double beginning_of_this_day = mktime(&date_tm);
		double beginning_of_prev_day = beginning_of_this_day - (24*60*60);

		printf("maria routine starts at %f. t1 = %f t2 = %f\n", now(), beginning_of_prev_day, beginning_of_this_day);
		int rv;
		if (not (rv = mysql_ping(im->mysql)) == 0)
		{	
			printf("ping returned nonzero: %i: errno %i: %s\n", rv, mysql_errno(im->mysql), mysql_error(im->mysql));
			im->reconnect();
		}
		mysql_query(im->mysql, "INSERT INTO `Farm`.`days` (`Date`, `Water consumption`, `Food time`, `Sun kwh`) VALUES ('2019-04-19', '1.2', '3.4', '5.67');");
		//double sleeptime = (24*60*60) - fmod(now(), (double)(24*60*60)) + 300; // 300 seconds after each new UTC day.
		double sleeptime = 10;
		printf("maria routine end. Will now sleep for %f seconds.\n", sleeptime);
		sleep(sleeptime);
		
	}
	mysql_close(im->mysql);
	mysql_library_end();
	printf("maria thread says byebye\n");
	return imvp;
}

void interface_maria::getIns()
{
	
}

void interface_maria::setOut(out*, float)
{

}
