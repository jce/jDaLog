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

#define MAX_SLEEPTIME 1000000 /* us */
#define MSG_SIZE_MAX 1024

#define DBG(...)
//#define DBG(...) { printf(__VA_ARGS__); printf("\n"); }

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

interface_S7::interface_S7(const string d, const string n, float i, const string _ipstr, S7_conntype c, uint16_t r, uint16_t s):interface(d, n, i), ipstr(_ipstr), conntype(c), rack(r), slot(s)
{
	ins["latency"] = new in(getDescriptor() + "_lt", getName() + " latency", "ms", 3);
	latency = ins["latency"];
}

interface_S7::~interface_S7()
{
	disconnect();
}

void interface_S7::connect()
{
	DBG("Connecting to PLC %s (%s)", getName().c_str(), ipstr.c_str());
    PLC = Cli_Create();
	switch(conntype) 
	{
		case S7_OP:
    		Cli_SetConnectionType(PLC, CONNTYPE_OP);
			DBG("CONNTYPE_OP");
			break;
		case S7_basic:
    		Cli_SetConnectionType(PLC, CONNTYPE_BASIC);
			DBG("CONNTYPE_BASIC");
			break;
		case S7_PG:
		default:
    		Cli_SetConnectionType(PLC, CONNTYPE_PG);
			DBG("CONNTYPE_PG");
			break;
	}
    Cli_ConnectTo(PLC, ipstr.c_str(), rack, slot);
}

void interface_S7::disconnect()
{
	DBG("PLC %s (%s) disconnecting", getName().c_str(), ipstr.c_str());
	Cli_Disconnect(PLC);	
	Cli_Destroy(&PLC);
	DBG("PLC %s (%s) disconnected", getName().c_str(), ipstr.c_str());
}

void interface_S7::start()
{
	init_schedule();
	connect();	
	run_flg = true;
	interface::start();
}

void interface_S7::run()
{
	uint8_t msg[MSG_SIZE_MAX];
	double t, tin, latency_start, latency_end;
	uint16_t db, first, last;
	while (run_flg)
	{
		last = 0;
		t = now_mt();
		tin = now();
		if( schedule )
		{
			db = schedule->db;
			first = schedule->start;
		}

		S7_io *transaction = NULL, *item;
		S7_io **sp = &schedule;
		// Take the subset of scheduled items that fit in the largest message and put them in 
		// a separate transaction list.
		// The second condition is to keep including short schedule items, even after a longer
		// schedule item would not fit any more.
		while (
				(*sp) and 
				(*sp)->next_scheduled_time <= t and 
				(*sp)->db == db and 
				(*sp)->start < first + MSG_SIZE_MAX
			  )
		{
			if ((*sp)->end < first + MSG_SIZE_MAX)
			{
				// moves item from sp to transaction, the next schedule item
				// gets its place in schedule. sp temains the same.

				// Calculate the new last byte / end / length.
				if (last < (*sp)->end)
					last = (*sp)->end;

				// Remove this item from the schedule.
				item = *sp;
				*sp = (*sp)->next;

				// Move (insert) this item to the transaction.
				item->next = transaction;
				transaction = item;
			}
			else
			{
				// This is a "skip", continue the schedule pointer. No
				// items get moved. The next item could be shorter and still fit 
				// in MSG_SIZE_MAX
				sp = & (*sp)->next;
			}
		}
	
		if( transaction )
		{	
			// Request the memory section for transaction.
			DBG("PLC %s (%s) requesting db %d, start %d, len %d", getName().c_str(), ipstr.c_str(), db, first, last-first+1);
			latency_start = now_mt();
			while ( Cli_DBRead(PLC, db, first, last-first+1, &msg) != 0 and run_flg)
			{
				disconnect();
				sleep(5);
				connect();
				latency_start = now_mt();
			}
			latency_end = now_mt();
			latency->setValue((latency_end - latency_start) * 1000, tin);
		}
		
		// Translate the message to inputs
		while ( transaction and run_flg )
		{
			// Process input
			if (not transaction->has_validbit or *(msg + transaction->validbyte - first) & (1 << transaction->validbit))
			{
				uint8_t *p = msg + transaction->key.byte - first;
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
			}			

			// reschedule the S7_io
			item = transaction;
			transaction = transaction->next;
			reschedule(item, t);
		}
	
		// Sleep until next scheduled time, or to a defined maximum time. This is to
		// detect run_flg regularly.
		if (run_flg)
		{	
			double sleeptime = MAX_SLEEPTIME;
			if (schedule)
				sleeptime = (schedule->next_scheduled_time - t) * 1000000;
			if (sleeptime > MAX_SLEEPTIME)
				sleeptime = MAX_SLEEPTIME;
			if (sleeptime < 0)
				sleeptime = 0;
			usleep(sleeptime);
		}
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
	while ( 
			(*i) 
			&&
			(
				(*i)->next_scheduled_time < si->next_scheduled_time 
				||
				(
					(*i)->next_scheduled_time == si->next_scheduled_time &&
					(*i)->db < si->db
				) 
				||
				(
					(*i)->next_scheduled_time == si->next_scheduled_time &&
					(*i)->db == si->db &&
					(*i)->start < si->start
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
	#define JSTR(_X_)	json_string_value(json_object_get(json, #_X_))
	#define JNR(_X_)	json_number_value(json_object_get(json, #_X_))
	#define JIN(_X_)	json_is_number(json_object_get(json, #_X_))
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
	float defaultinterval = JNR(interval);
	if (not defaultinterval)
		defaultinterval = 1;
	float defaultvalidtime = JNR(valid_time);
	if (not id or not ip)
		return;
	uint16_t default_validbyte = 0, default_validbit = 0;
	bool has_default_validbit = JIN(validbit);
	if (has_default_validbit)
	{
		default_validbyte = JNR(validbit);
		default_validbit = JNR(validbit) * 10 - default_validbyte * 10;
	}	

	interface_S7 *S7 = new interface_S7(id, name, 1, ip, conntype, rack, slot);

	#undef JIN
	#undef JNR
	#undef JSTR
	size_t idx;
	json_t *in_j;
	map<S7_key, S7_io> inmap;
	json_array_foreach(json_object_get(json, "in"), idx, in_j)
	{
		#define JSTR(_X_)	json_string_value(json_object_get(in_j, #_X_))
		#define JNR(_X_)	json_number_value(json_object_get(in_j, #_X_))
		#define JIN(_X_)	json_is_number(json_object_get(in_j, #_X_))
		const char *regtypestr = JSTR(type);
		S7_regtype type = S7_int16;
		for( int j = S7_bool; j < S7_regnum; j++)
			if (strcmp(regtypestr, S7_regtype_str[j]) == 0 ) type = (S7_regtype) j;
		int db = JNR(db);
		if (not db)
			db = defaultdb;
		int byte = JNR(byte);
		int bit = JNR(bit);
		float interval = JNR(interval);
		if (not interval)
			interval = defaultinterval;
		float validtime = JNR(valid_time);
		if (not validtime)
			validtime = defaultvalidtime;
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
		uint16_t validbyte = 0, validbit = 0;
		bool has_validbit = false;
		if (has_default_validbit)
		{
			has_validbit = true;
			validbyte = default_validbyte;
			validbit = default_validbit;
		}
		if (JIN(validbit))
		{
			has_validbit = true;
			validbyte = JNR(validbit);
			validbit = JNR(validbit) * 10 - validbyte * 10;
		}	
		if (json_is_false(json_object_get(in_j, "validbit")))
		{
			has_validbit = false;
		}
		uint16_t start = byte;	// The block to read starts at this byte
		if (has_validbit and validbyte < start)
			start = validbyte;
		uint16_t end = byte + S7_regtype_len[type] - 1; // End is the last byte that has to be read.
		if (has_validbit and validbyte > end)
			end = validbyte;
		uint16_t len = end - start + 1; // The length of the block to be read.

		if (len > MSG_SIZE_MAX)
		{
			printf("in %s (%s) wants an atomic read larger than the maximum message size (%d bytes)\n", inname, inid, MSG_SIZE_MAX);
			continue;
		} 

		#undef JIN
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
			S7->ios[key].db = db;
			S7->ios[key].start = start;
			S7->ios[key].end = end;
			S7->ios[key].len = len;
			S7->ios[key].read_interval = interval;
			S7->ios[key].a = a;
			S7->ios[key].b = b;
			S7->ios[key].key = key;
			S7->ios[key].has_validbit = has_validbit;
			S7->ios[key].validbyte = validbyte;
			S7->ios[key].validbit = validbit;
			
			S7->ios[key].i = new in(inid_full, inname_full, unit, decimals);
			if (validtime)
				S7->ios[key].i->set_valid_time(validtime);
			S7->ins[inid_full] = S7->ios[key].i;
		}
	}
}

