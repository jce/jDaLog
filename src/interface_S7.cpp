//#define byte byte_override
//#include "snap7.h"
//#undef byte

#include "interface_S7.h"
#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "out.h"
#include "endian.h" 

//#define DBG(...)
#define DBG(...) { printf(__VA_ARGS__); printf("\n"); }

using namespace std;

const char* S7_conntype_str[S7_connnum] = 
	{
		"PG",
		"OP",
		"basic"
	};

const char* S7_regtype_str[S7_regnum] = 
	{
		"bool",
		"uint8",
		"int8",
		"uint16",
		"int16",
		"uint32",
		"int32",
		"uint64",
		"int64",
		"float32",
		"float64"
	};

const int S7_regtype_len[S7_regnum] = 
	{
		1, 1, 1, 2, 2, 4, 4, 8, 8, 4, 8
	};

bool operator<(const S7_key& l, const S7_key& r)
{
	return  (l.db < r.db) || \
			(l.db == r.db && l.byte < r.byte) || \
			(l.db == r.db && l.byte == r.byte && l.bit < r.bit);
}

S7_io::~S7_io()
{
	delete i;
}

interface_S7::interface_S7(const string d, const string n, float i, const string _ipstr, S7_conntype c, uint16_t r, uint16_t s):interface(d, n, i), ipstr(_ipstr), conntype(c), rack(r), slot(s)
{
	ins["latency"] = new in(getDescriptor() + "_lt", getName() + " latency", "ms", 3);
	latency = ins["latency"];
}

interface_S7::~interface_S7()
{
	run_flg = false;
}

void interface_S7::getIns()
{
}

void dbg_list_item(S7_io *list)
{
		DBG("next: %p, key: db%d, %d.%d, type: %d, interval: %f, scheduled: %f, id: %s, name: %s, unit: %s, decimals: %d, a: %f, b: %f, in: %p", list->next, list->key.db, list->key.byte, list->key.bit, list->type, list->read_interval, list->next_scheduled_time, list->i->getDescriptor().c_str(), list->i->getName().c_str(), list->i->getUnits().c_str(), list->i->getDecimals(), list->a, list->b, list->i);
}

void dbg_list(S7_io *list)
{
	DBG("displaying list at %p", list);
	while (list)
	{
		dbg_list_item(list);
		list = list->next;
	}
}

void interface_S7::start()
{
	init_schedule();
	//dbg_list(schedule);
    PLC = Cli_Create();
	switch(conntype) 
	{
		case S7_OP:
    		Cli_SetConnectionType(PLC, CONNTYPE_OP);
			break;
		case S7_basic:
    		Cli_SetConnectionType(PLC, CONNTYPE_BASIC);
			break;
		case S7_PG:
		default:
    		Cli_SetConnectionType(PLC, CONNTYPE_PG);
			break;
	}
    Cli_ConnectTo(PLC, ipstr.c_str(), rack, slot);
	run_flg = true;
	interface::start();
}


void interface_S7::run()
{
	#define MSG_SIZE_MAX 1024
	
	uint8_t msg[MSG_SIZE_MAX];
	double t, tin, latency_start, latency_end;
	uint16_t db, start, len;
	while (run_flg)
	{
		t = now_mt();
		tin = now();
		if (t < schedule->next_scheduled_time)
			t = schedule->next_scheduled_time;	// Prevent double calls if sleep returns early.
		db = schedule->key.db;
		start = schedule->key.byte;
		#define END (schedule->key.byte + S7_regtype_len[schedule->type])
		S7_io *transaction = NULL, *item;

		// Take the subset of scheduled items that fit in the largest message and put them in 
		// a separate transaction list.
		while (schedule and schedule->next_scheduled_time <= t and schedule->key.db == db and (END - start) <= MSG_SIZE_MAX)
		{
			len = END - start;
			item = transaction;
			transaction = schedule;
			schedule = schedule->next;
			transaction->next = item;
		}
		
		//DBG("Requesting db %d, start %d, len %d", db, start, len);	
		//dbg_list(transaction);
		// Request the memory section for transaction.
		latency_start = now_mt();
		while ( Cli_DBRead(PLC, db, start, len, &msg) != 0 and run_flg)
		{
			DBG("Reconnect");
			Cli_Disconnect(PLC);	
			sleep(1);
			Cli_ConnectTo(PLC, ipstr.c_str(), rack, slot);
			latency_start = now_mt();
		}
		latency_end = now_mt();
		latency->setValue((latency_end - latency_start) * 1000, tin);

		// Translate the message to inputs
		while ( transaction and run_flg )
		{
			// Process input
			uint8_t *p = msg + transaction->key.byte - start;
			double val;
			switch (transaction->type)
			{
				case S7_bool:	val = (bool) (*p & ( 1 << transaction->key.bit)); break;
				case S7_uint8: 	val = (uint8_t) *p; break;
				case S7_int8:	val = (int8_t) *p; break;
				case S7_uint16:	val = (uint16_t) be16toh( *(uint16_t*) p ); break;
				case S7_int16:	val = (int16_t) be16toh( *(uint16_t*) p ); break;
				case S7_uint32:	val = (uint32_t) be32toh( *(uint32_t*) p ); break;
				case S7_int32:	val = (int32_t) be32toh( *(uint32_t*) p ); break;
				case S7_uint64:	val = (uint64_t) be16toh( *(uint64_t*) p ); break;
				case S7_int64:	val = (int64_t) be16toh( *(uint64_t*) p ); break;
				case S7_float32:{uint32_t x; x = be32toh ( *(int32_t*) p); val = * (float*) &x;} break;
				case S7_float64:{uint64_t x; x = be64toh ( *(int64_t*) p); val = * (double*) &x;} break;
				default: break;
			}
			val = transaction->a * val + transaction->b;
			transaction->i->setValue(val, tin);
			
			// reschedule the S7_io
			item = transaction;
			transaction = transaction->next;
			reschedule(item, t);
		}

		// Sleep until next scheduled time
		if (run_flg)
			usleep((schedule->next_scheduled_time - t) * 1000000);
	}
}

double interface_S7::next_multiple(double val, double interval)
{
	double n = floor(val / interval);
	return interval * (n + 1);
}

void interface_S7::init_schedule()
{
	
	double t = now_mt();
	for (auto i = interface_S7::ios.begin(); i != interface_S7::ios.end(); i++)
	{
		//dbg_list_item( &(i->second) );
		reschedule( &(i->second), t );
	}
}

void interface_S7::reschedule(S7_io *si, double t)
{
	// Remove from schedule
	S7_io **i = &schedule;
	while ( (*i) && *i != si)
		i = & (*i)->next;
	if (*i)
	{
		*i = si->next;
	}

	// Calculate next time
	si->next_scheduled_time = next_multiple(t, si->read_interval);

	// Add at the appropriate place in the schedule
	i = &schedule;
	while ( (*i) &&
			(
				(*i)->next_scheduled_time < si->next_scheduled_time ||
				(
					(*i)->next_scheduled_time == si->next_scheduled_time &&
					(*i)->key < si->key
				)
			)
		  )
	{
		i = & (*i)->next;
	}
	si->next = (*i);
	(*i) = si;
}

void interface_S7_from_json(const json_t *json)
{
	#define JSTR(_X_)	json_string_value(json_object_get(json, #_X_));
	#define JNR(_X_)	json_number_value(json_object_get(json, #_X_));
	const char *id = JSTR(id);
	const char *name = JSTR(name);
	if (not name)
		name = id;
	const char *ip = JSTR(ip);
	const char *conntypestr = JSTR(conntype);
	S7_conntype conntype = S7_PG;
	for( int j = S7_PG; j < S7_connnum; j++)
		if (strcmp(conntypestr, S7_conntype_str[j]) == 0 ) conntype = (S7_conntype) j;
	const int rack = JNR(rack);
	const int slot = JNR(slot);
	int defaultdb = JNR(db);
	if (not defaultdb)
		defaultdb = 1;
	int defaultinterval = JNR(interval);
	if (not defaultinterval)
		defaultinterval = 1;
	if (not id or not ip)
		return;

	interface_S7 *S7 = new interface_S7(id, name, 1, ip, conntype, rack, slot);

	#undef JNR
	#undef JSTR
	size_t idx;
	json_t *in_j;
	map<S7_key, S7_io> inmap;
	json_array_foreach(json_object_get(json, "in"), idx, in_j)
	{
		#define JSTR(_X_)	json_string_value(json_object_get(in_j, #_X_));
		#define JNR(_X_)	json_number_value(json_object_get(in_j, #_X_));
		const char *regtypestr = JSTR(type);
		S7_regtype type = S7_int16;
		for( int j = S7_bool; j < S7_regnum; j++)
			if (strcmp(regtypestr, S7_regtype_str[j]) == 0 ) type = (S7_regtype) j;
		int db = JNR(db);
		if (not db)
			db = defaultdb;
		int byte = JNR(byte);
		int bit = JNR(bit);
		int interval = JNR(interval);
		if (not interval)
			interval = defaultinterval;
		const char *inid = JSTR(id);
		const char *inname = JSTR(name);
		if (not inname)
			inname = inid;
		const char* unit_c = JSTR(unit);
		string unit( unit_c? unit_c : "" );
		const int decimals = JNR(decimals);
		double a = JNR(a);
		if (a == 0)
			a = 1;
		const double b = JNR(b);
		#undef JNR
		#undef JSTR

		string inid_full = (string) id + "_" + inid;
		string inname_full = (string) name + " " + inname;

		if (inid and not get_in(inid_full))
		{
			S7_key key;
				key.db = db;
				key.byte = byte;
				key.bit = bit;
			S7->ios[key].key = key;
			S7->ios[key].type = type;
			S7->ios[key].read_interval = interval;
			S7->ios[key].a = a;
			S7->ios[key].b = b;
			S7->ios[key].key = key;
			S7->ios[key].i = new in(inid_full, inname_full, unit, decimals);
			S7->ios[key].i->set_valid_time(interval * IN_VALIDTIME_SCAN_MULTIPLY);
		}
	}
}
























