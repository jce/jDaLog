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
		"float16",
		"float32",
		"float64"
	};

const int S7_regtype_len[S7_regnum] = 
	{
		1, 1, 1, 2, 2, 4, 4, 8, 8, 2, 4, 8
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
}

interface_S7::~interface_S7()
{
}

void interface_S7::getIns()
{
	/*
	interaction_counter++;
	interaction_30 = interaction_counter == 30;
	if (interaction_30)
		interaction_counter = 0;

	uint8_t data[READ_SIZE];
	uint32_t rv;
	
	// Read part
	double start = now();
    rv = Cli_DBRead(PLC, DB_NR, 0, READ_SIZE, &data);
	if (rv != 0) 
	{
		DBG("Reading PLC data failed, rv=%i", rv);
		Cli_Disconnect(PLC);
		Cli_ConnectTo(PLC, _ipstr.c_str(), 0, 2);
        	rv = Cli_DBRead(PLC, DB_NR, 0, READ_SIZE, &data);
		}
	double end = now();

	double t = (start + end) / 2;
	ins["latency"]->setValue((end-start)/1*1000, t); // 1 request, scale = ms
	if(rv == 0)
	{
		DBG("data read: %016lx", * (uint64_t*) data);

		int32_t runtime = be32toh(*(uint32_t*) (data + 0));
		if (runtime >= RUNTIME_MIN and runtime > last_runtime)
		{
			#define BOOL(_DBOFFSET_, _BITOFFSET_, _DESCR_, _NAME_) \
				ins[_DESCR_]->setValue((bool) (data[_DBOFFSET_] & (1 << _BITOFFSET_)) , t);
			#define I16(_DBOFFSET_, _DESCR_, _NAME_, _UNIT_, _DECIMALS_, _FACTOR_) \
				ins[_DESCR_]->setValue( (int16_t) be16toh(*(uint16_t*) (data+_DBOFFSET_)) * _FACTOR_, t);
			#define I32(_DBOFFSET_, _DESCR_, _NAME_, _UNIT_, _DECIMALS_, _FACTOR_) \
				ins[_DESCR_]->setValue( (int32_t) be32toh(*(uint32_t*) (data+_DBOFFSET_)) * _FACTOR_, t);
			READVALUES	
			#undef I32
			#undef I16
			#undef BOOL
		}
		last_runtime = runtime;
	}
*/
}

void interface_S7::start()
{
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
	// build_outreg();
	// link_adj_reg_contexts();
	// init_schedule();
	interface::start();
}

void interface_S7::run()
{

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
	*i = si->next;

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
			S7_io io;
				io.key = key;
				io.type = type;
				io.read_interval = interval;
				io.i = new in(inid_full, inname_full, unit, decimals);
				io.i->set_valid_time(interval * IN_VALIDTIME_SCAN_MULTIPLY);
				io.a = a;
				io.b = b;
			S7->ios[key] = io;
		}
	}
}
























