#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "main.h"
#include "mysql/mysql.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "out.h"
#include "unistd.h" // sleep

using namespace std;

#define HOST		"localhost"
#define PORT		3306	
#define DATABASE	"tcfarmcontrol"
#define USER		"tcfarmcontrol"
#define PASS		"technochicken123!@"
#define QSIZE		100000

bool firstrun = true;

void query(MYSQL *m, const char *q)
{
	printf("%s\n", q);
	mysql_query(m, q);
	printf("%u %s\n", mysql_errno(m), mysql_error(m));
}

void create_index_table(MYSQL *m)
{
	// Construction queries are separated out per column, for if the table exists but one column does not, it is not added.
	//query(m, "CREATE TABLE IF NOT EXISTS  `__index` ( `name` TINYTEXT NOT NULL, `long_name` TINYTEXT NULL, `note` TEXT NULL DEFAULT '', PRIMARY KEY (`name`(100)) )COMMENT='comment' COLLATE='utf8mb4_general_ci';");
	query(m, "CREATE TABLE IF NOT EXISTS  `__index` ( `name` TINYTEXT NOT NULL, PRIMARY KEY (`name`(100)) ) COLLATE='utf8mb4_general_ci'");
	query(m, "ALTER TABLE `__index` ADD COLUMN IF NOT EXISTS `long_name` TINYTEXT NULL DEFAULT NULL");
	query(m, "ALTER TABLE `__index` ADD COLUMN IF NOT EXISTS `unit` TINYTEXT NULL DEFAULT NULL");
	query(m, "ALTER TABLE `__index` ADD COLUMN IF NOT EXISTS `decimals` TINYINT NULL DEFAULT NULL");
	query(m, "ALTER TABLE `__index` ADD COLUMN IF NOT EXISTS `note` TEXT NULL DEFAULT NULL");
}

void create_in_table(MYSQL *m, const char* name)
{
    char buf[QSIZE];
    snprintf(buf, QSIZE, "CREATE TABLE IF NOT EXISTS `%s` ( `time` DECIMAL(17,6) NOT NULL, `value` FLOAT NULL, PRIMARY KEY (`time`) ) COLLATE='utf8mb4_general_ci'", name);
    query(m, buf);
}

void records_to_in_table(MYSQL *m, const char *name, map<double, float> &data)
{
	char buf[QSIZE];
	size_t len = 0;
	//REPLACE INTO `tcfarmcontrol`.`asdf` (`time`, `value`) VALUES ('123', '44'), ('1234', '444');
	len += snprintf(buf, QSIZE-len, "REPLACE INTO `tcfarmcontrol`.`%s` (`time`, `value`) VALUES ", name);

	for (auto i = data.begin(); i != data.end(); i++)
	{
		len += snprintf(buf+len, QSIZE-len, "('%f', '%f'),", i->first, i->second);	
	}

	// Remove last comma
	if (data.size() > 1)
	{
		len -= 1;
		buf[len] = 0;
	}
		
	query(m, buf);
}

void in_to_index(MYSQL *m, in *i)
{
	char buf[QSIZE];
	snprintf(buf, QSIZE, "INSERT INTO `tcfarmcontrol`.`__index` (`name`) VALUES ('%s')", i->getDescriptor().c_str());
	query(m, buf);
	snprintf(buf, QSIZE, "UPDATE `tcfarmcontrol`.`__index` SET `long_name`='%s' WHERE `name`='%s'", i->getName().c_str(), i->getDescriptor().c_str());
	query(m, buf);
	snprintf(buf, QSIZE, "UPDATE `tcfarmcontrol`.`__index` SET `unit`='%s' WHERE `name`='%s'", i->getUnits().c_str(), i->getDescriptor().c_str());
	query(m, buf);
	snprintf(buf, QSIZE, "UPDATE `tcfarmcontrol`.`__index` SET `decimals`='%d' WHERE `name`='%s'", i->getDecimals(), i->getDescriptor().c_str());
	query(m, buf);
	snprintf(buf, QSIZE, "UPDATE `tcfarmcontrol`.`__index` SET `note`='%s' WHERE `name`='%s'", i->getNote().c_str(), i->getDescriptor().c_str());
	query(m, buf);
}

void in_to_maria()
{
	printf("running floatlog_to_maria function.\n");fflush(stdout);
	// Brutal program that goes through all the binary data, by hour, and verifies if the number of records agrees with what is in the database. If not, it adds / overwrites all records.

	MYSQL *m = mysql_init(NULL);
	mysql_real_connect(m, HOST, USER, PASS, DATABASE, PORT, NULL, 0);
	my_bool reconnect = 1;
	mysql_options(m, MYSQL_OPT_RECONNECT, &reconnect);

	// First create one table with all ins
	if(firstrun)
	{
		firstrun = false;	
		create_index_table(m);
		map<string, in*>::iterator i;
		for(i = inmap.begin(); i != inmap.end(); i++)
		{
			in_to_index(m, i->second);
			create_in_table(m, i->second->getDescriptor().c_str());
			map<double, float> data;
			//test[200] = 50;
			//test[201.539] = 51.234;
			size_t num_records = i->second->getNumRecords();
			const unsigned int block = 1024;
			for (size_t b = 0; b < num_records; b += block) 
			{
				i->second->getRecords(data, b, block);
				records_to_in_table(m, i->second->getDescriptor().c_str(), data);
				data.clear();
			}
		}
	}
	mysql_close(m);
	printf("finished floatlog_to_maria function.\n");fflush(stdout);
}

/*
double interface_maria::get_last_logged_day_from_db()
{
	// The query is 
	// SELECT max(DATE) FROM `tcFarmControl days`;
	//printf("Trying query\n");
	char query[2048] = "SELECT max(DATE) FROM `tcFarmControl days`";
	int rv = mysql_query(mysql, query);
	if (rv)
	{
		printf("Received %i from mysql_query(). Errno %i, error %s\n", rv, mysql_errno(mysql), mysql_error(mysql));
		
		return -1;
	}
	//printf("select query successful\n");
	MYSQL_RES *result = mysql_store_result(mysql);
	if (not result) return -1;
	//printf("Result received\n");

	int num_fields =mysql_num_fields(result);
	if (num_fields == 1)
	{
		//printf("1 field in result\n");
		MYSQL_ROW row = mysql_fetch_row(result);
		if(row)
		{
			//printf("There is a row. it has %i fields\n", mysql_num_fields(result));
			//printf("row[0] is %s\n", row[0]);
			tm date_tm = {};
			date_tm.tm_isdst = -1;
			sscanf(row[0], "%d-%d-%d", &date_tm.tm_year, &date_tm.tm_mon, &date_tm.tm_mday);
			date_tm.tm_year -= 1900;	// http://www.cplusplus.com/reference/ctime/tm/
			date_tm.tm_mon -= 1;
			print_tm(date_tm);
			time_t ts = mktime(&date_tm);
			print_tm(date_tm);
			//printf("The timestamp with this is %li\n", ts);
			tm local_tm;
			localtime_r(&ts, &local_tm);
			//printf("Which translates back to %i-%i-%i %i:%i:%i\n", local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday, local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);

			printf("%s -> %s -> %li -> %i-%i-%i %i:%i:%i\n", query, row[0], ts, local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday, local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);

			return (double)ts;	

				
		}	
		
	}
	mysql_free_result(result);
	return -1;
}

void interface_maria::thread_fnc()
{
	connect();
	//time_t utctime;
	for(;;)
	{
	//	utctime = time(0);
	//	tm date_tm;
	//	localtime_r(&utctime, &date_tm);
	//	date_tm.tm_sec = 0;
	//	date_tm.tm_min = 0;
	//	date_tm.tm_hour = 0;
	//	double beginning_of_this_day = mktime(&date_tm);
	//	double beginning_of_prev_day = beginning_of_this_day - (24*60*60);
		double lastday = get_last_logged_day_from_db();

		while (lastday + 2 * (24*60*60) < now() - 300)
		{
			lastday += (24*60*60);
			write_one_day_record(lastday);
			pthread_yield();
		}		

		//printf("maria routine starts at %f. t1 = %f t2 = %f\n", now(), beginning_of_prev_day, beginning_of_this_day);
		
		//int rv;
		//if (not (rv = mysql_ping(mysql)) == 0)
		//{	
		//	printf("ping returned nonzero: %i: errno %i: %s\n", rv, mysql_errno(mysql), mysql_error(mysql));
		//	reconnect();
		//}
		//mysql_query(mysql, "INSERT INTO `Farm`.`days` (`Date`, `Water consumption`, `Food time`, `Sun kwh`) VALUES ('2019-04-19', '1.2', '3.4', '5.67');");
		double sleeptime = 3600;
		//printf("maria routine end. Will now sleep for %f seconds.\n", sleeptime);
		sleep(sleeptime);
	


	
	}
	mysql_close(mysql);
	mysql_library_end();
	//printf("maria thread says byebye\n");
}

bool in_to_day_diff(in* inp, double from, float &diff)
{
	double to = from + (24*60*60);
	float v1, v2;
	double t1, t2;
	if (inp->get_value_at(from, v1, t1) and inp->get_value_at(to, v2, t2))
		if(abs(t1-from) < 300 and abs(t2 - to) < 300)
		{
			diff = v2 - v1;
			return true;
		}
	return false;
}
bool in_to_day_diff(string name, double from, float &diff)
{
	in *inp = get_in(name);
	if (inp)
	{
		return in_to_day_diff(inp, from, diff);
	}
	return false;
}

void interface_maria::write_one_day_record(double from)
{
	#define QSIZE 2048
	char query[QSIZE];
	char date[64];
	float val;
	double ts;	
	in *inp;
	double to = from + (24*60*60);

	time_t from_tt = from;
	tm local_tm;
	localtime_r(&from_tt, &local_tm);
	snprintf(date, QSIZE, "%i-%02i-%02i", local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday);

	// Create query for the record
	// INSERT INTO `Farm`.`tcFarmControl days` (`date`) VALUES ('2019-06-15');
	snprintf(query, QSIZE, "INSERT INTO `Farm`.`tcFarmControl days` (`date`) VALUES ('%s');", date);
	mysql_query(mysql, query);
	printf("%s\n", query);

	if (in_to_day_diff("S1200_shw", from, val))
	{
		snprintf(query, QSIZE, "UPDATE `Farm`.`tcFarmControl days` SET `sheep water`='%f' WHERE `date`='%s';", val, date);
		mysql_query(mysql, query);
		printf("%s\n", query);
	}

	if (in_to_day_diff("S1200_shf", from, val))
	{
		val = val / 3600.0; // Seconds to hours
		snprintf(query, QSIZE, "UPDATE `Farm`.`tcFarmControl days` SET `sheep food time`='%f' WHERE `date`='%s';", val, date);
		mysql_query(mysql, query);
		printf("%s\n", query);
	}

	if (in_to_day_diff("solarlog_tep", from, val))
	{
		snprintf(query, QSIZE, "UPDATE `Farm`.`tcFarmControl days` SET `solar production`='%f' WHERE `date`='%s';", val, date);
		mysql_query(mysql, query);
		printf("%s\n", query);
	}

	inp = get_in("rwl_wv");
	if (inp)
		if (inp->get_value_at(from+12*60*60, val, ts) and (from - ts < 300))
			{
			snprintf(query, QSIZE, "UPDATE `Farm`.`tcFarmControl days` SET `water level`='%f' WHERE `date`='%s';", val, date);
			mysql_query(mysql, query);
			printf("%s\n", query);
			}

	inp = get_in("darksky_temp");
	if (inp)
	{
		map<double, float> data;
		inp->getData(data, from, to);
		if(data.size() > 400)
		{
			float min = data.begin()->second;
			float max = data.begin()->second;
			long count = 0;
			double sum = 0.0;
			for(map<double, float>::iterator i = data.begin(); i != data.end(); i++)
			{
				if (i->second < min) min = i->second;
				if (i->second > max) max = i->second;
				count += 1;
				sum += i->second;
			}
			snprintf(query, QSIZE, "UPDATE `Farm`.`tcFarmControl days` SET `min temp`='%f', `avg temp`='%f', `max temp`='%f' WHERE `date`='%s';", min, sum/count, max, date);
			mysql_query(mysql, query);
			printf("%s\n", query);
		}
	}

	inp = get_in("S1200_rac");
	if (inp)
		if (inp->get_value_at(from+12*60*60, val, ts) and (from - ts < 300))
			{
			snprintf(query, QSIZE, "UPDATE `Farm`.`tcFarmControl days` SET `rain`='%f' WHERE `date`='%s';", val, date);
			mysql_query(mysql, query);
			printf("%s\n", query);
			}
	if (in_to_day_diff("S1200_rac", from, val))
	{
		snprintf(query, QSIZE, "UPDATE `Farm`.`tcFarmControl days` SET `rain`='%f' WHERE `date`='%s';", val, date);
		mysql_query(mysql, query);
		printf("%s\n", query);
	}

}

void interface_maria::getIns()
{
	
}

void interface_maria::setOut(out*, float)
{

}
*/

