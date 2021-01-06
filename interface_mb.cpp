#include "floatLog.h"
#include "interface.h"
#include "interface_mb.h"
#include "main.h"
#include "math.h" // pow()
#include "modbus/modbus.h"
#include "out.h"
#include "stdio.h"
#include <string>
#include "string.h"
#include "stringStore.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include <unistd.h>		// sleep

//#define debug

using namespace std;

bool operator<(const mb_key& l, const mb_key& r)
{
	return  (l.id <  r.id) || \
			(l.id == r.id && l.regtype <  r.regtype) || \
			(l.id == r.id && l.regtype == r.regtype && l.offset <  r.offset);
}

reg_context::~reg_context()
{
	if (i)
		delete i;
}

const char* mb_regtype_str[mb_regtype_num] = {"none", "coil", "status", "input", "holding"};
const char* mb_datatype_str[mbd_num] = {"none", "bool", "uint16", "int16", "uint32", "int32", "uint64", "int64", "float16", "float32", "float64"};
const uint8_t mb_datatype_len[mbd_num] = { 1,	1,		1,		  1,		2,		 2,		  4,		4,		 1,			2,		   4};

interface_mb::interface_mb(const string d, const string n, const string ipstr, uint16_t _port):interface(d, n, 1), _ipstr(ipstr), port(_port)
{
}

interface_mb::~interface_mb()
{
}

void interface_mb::getIns()
{
	printf("should not happen!\n");
}

void interface_mb::setOut(out* o, float v)
{
	/*
	if (!globalControl) return;
	makeModbusContext();
	bool newState = true;
	if (v < 0.5) newState = false;
	if(ctx)
		for (uint16_t i = 0; i<8; i++)
			if (o == DO[i] and newState != DO[i]->getValue())
				if(modbus_connect(ctx) != -1)
					{
					modbus_write_bit(ctx, 16+i, newState);
					modbus_close(ctx); 
					getIns();
					}
	*/
}

void interface_mb::start()
{
	build_outreg();
	link_adj_reg_contexts();
	init_schedule();
	interface::start();
}

void interface_mb::run()
{
	// Pick off the first register from the queue. Then trace back and forwards if other registers are also eligible to be fetched.
	#define MAX_BITS 2000
	#define MAX_REGS 125
	#define ACCEPT_EARLY 0.001

	uint8_t bits[MAX_BITS];
	uint16_t regs[MAX_REGS];
	double t;
	mb_key key;
	mb_offset num;
	mb_offset max_fetch;
	reg_context *start, *end, *reg;	// first item this fetch, last item this fetch, some loop temp value.
	int rv;
	modbus_t *ctx = modbus_new_tcp(_ipstr.c_str(), port);
	modbus_set_response_timeout(ctx, 1, 0);
	bool conmem = 1;	// To display errors only once.
	run_flg = true;
	
	// Initial connection loop. Try every second.
	while (run_flg && modbus_connect(ctx) != 0)
	{
		if (conmem)
		{
			printf("Modbus connection to %s:%d failed. Errno %d: %s\n", _ipstr.c_str(), port, errno, strerror(errno));
			conmem = 0;
		}
		sleep(1);
	}
	conmem = 1;

	while(run_flg)
	{
		t = now_mt();
		start = schedule;
		end = schedule;
		num = 1;

		if (t < schedule->time)
		{
			//printf("sleeping %f\n", schedule->time - t);
			usleep((schedule->time - t) * 1000000);
		}

		key = schedule->key;
		if (key.regtype == mb_coil || key.regtype == mb_status)
			max_fetch = MAX_BITS;
		else
			max_fetch = MAX_REGS;

		t = now_mt() + ACCEPT_EARLY;
		// Find the first item
		while (num < max_fetch && start->adj_before && t > start->adj_before->time)
		{
			num++;
			start = start->adj_before;
		}
		// Find the last item
		while (num < max_fetch && end->adj_after && t > end->adj_after->time)
		{
			num++;
			end = end->adj_after;
		}

		// Read
		//printf("Reading id-type-offset num %d %s %d %d\n", key.id, mb_regtype_str[start->key.regtype], key.offset, num);
		modbus_set_slave(ctx, key.id);
		switch(key.regtype)
		{
			case mb_coil:
				rv = modbus_read_bits(ctx, start->key.offset, num, bits);
				break;
			case mb_status:
				rv = modbus_read_input_bits(ctx, start->key.offset, num, bits);
				break;
			case mb_input:
				rv = modbus_read_input_registers(ctx, start->key.offset, num, regs);
				break;
			case mb_holding:
				rv = modbus_read_registers(ctx, start->key.offset, num, regs);
				break;
			case mb_none:
			default:
				rv = 0;
				break;
		}
		//printf("rv: %d\n", rv);

		// To in.
		int i;
		reg = start;
		if (rv == num)
		{
			t = now();
			if (key.regtype == mb_coil || key.regtype == mb_status)
				for (i = 0; i != num; i++)
				{
					reg->i->setValue(bits[i], t);
					reg = reg->adj_after;
				}
			if (key.regtype == mb_input || key.regtype == mb_holding)
				for (i = 0; i != num; i++)
				{
					reg->i->setValue(regs[i], t);
					reg = reg->adj_after;
				}
			if (!conmem)
			{
				printf("Modbus connection restored.\n");
				conmem = 1;
			}
		}
		else
		{
			for (i = 0; i != num; i++)
			{
				reg->i->setValid(false);
				reg = reg->adj_after;
			}
			if (conmem)
			{
				printf("Modbus communication with %s:%d failed. Errno %d: %s\n", _ipstr.c_str(), port, errno, strerror(errno));
				conmem = 0;
			}
		}
				
		// Reschedule
		t = now_mt();
		for (i = 0; i != num; i++)
		{
			start->time = next_multiple(t, start->interval);
			reschedule(start);
			start = start->adj_after;
		}
	}
	modbus_free(ctx);
} 

double interface_mb::next_multiple(double val, double interval)
{
	double n = floor(val / interval);
	return interval * (n + 1);
}

void interface_mb::build_outreg()
{
	for( auto key = reg.begin(); key != reg.end(); key++)
	{
		out *o = key->second.o;
		if (o)
			outreg[o] = key->first;
	}
}

void interface_mb::link_adj_reg_contexts()
{
	// l and r from left and right in the comparison.
	auto l = reg.begin();
	for (auto r = reg.begin(); r != reg.end(); r++)
	{
		if ( (l->first.id == r->first.id) && (l->first.regtype == r->first.regtype) && (l->first.offset + 1 == r->first.offset) )
		{
			// These two are adjacent.
			l->second.adj_after = & r->second;
			r->second.adj_before = & l->second;
		}
	l = r;	
	}
}

void interface_mb::init_schedule()
{
	for( auto key = reg.rbegin(); key != reg.rend(); key++)
	{
		reg_context *si = &key->second;
		si->next = schedule;
		schedule = si;
		si->time = 0.0;
	}
}

void printsch(reg_context **x)
{
	while (*x)
	{
		printf("%p -> %p\n", *x, (*x)->next);
		x = & (*x)->next;
	}
}

void interface_mb::reschedule(reg_context *si)
{
	// Remove the item from where it is in the list.
	reg_context **i = &schedule;
	while (*i != si)
		i = & (*i)->next;
	*i = si->next;

	// Add the item at the appropriate place
	i = &schedule;
	while ( (*i) && (*i)->time <= si->time)
		i = & (*i)->next;
	si->next = (*i); 
	(*i) = si;
}

void interface_mb_from_json(const char *ifid, const char *ifname, json_t *json)
{
	if (!ifid)
	{
		printf("interface_mb: no id given.\n");
		return;
	}	
	if (!ifname)
		ifname = ifid;
	const char* ipstr = json_string_value(json_object_get(json, "address"));
	if (!ipstr)
	{
		printf("interface_mb: no address given.\n");
		return;
	}
	json_t *devices_j = json_object_get(json, "device");
	if (!devices_j)
	{
		printf("interface_mb: no devices given.\n");
		return;
	}

	uint16_t port = MODBUS_TCP_DEFAULT_PORT;
	json_t *jport = json_object_get(json, "port");
	if (json_is_integer(jport))
		port = json_integer_value(jport);

	json_t *main_scan_j;
	float main_scan;
	main_scan_j = json_object_get(json, "scan");
	if (main_scan_j)
		main_scan = json_number_value(main_scan_j);	
	
	interface_mb *mb = new interface_mb(ifid, ifname, ipstr, port);

	// Reading the individual registers
	// "device":{
	//	"1":{
	//   "hr0004":
	//	  {}, optionally name, id, scan
	const char *device, *reg, *did, *dname, *rid, *rname;
	json_t *device_j, *reg_j;
	json_object_foreach(devices_j, device, device_j)
	{
		json_t *dev_scan_j;
		float dev_scan;
		dev_scan_j = json_object_get(device_j, "scan");
		if (dev_scan_j)
			dev_scan = json_number_value(dev_scan_j);	

		did = json_string_value(json_object_get(device_j, "id"));
		if (!did)
			did = device;

		dname = json_string_value(json_object_get(device_j, "name"));
		if (!dname)
			dname = did;

		json_object_foreach(device_j, reg, reg_j)
		{
			bool ok = true;
			mb_id id;
			ok &= sscanf(device, "%hhd", &id);
			mb_regtype regtype = mb_none;
			if (strncmp(reg, "co", 2) == 0)	regtype = mb_coil;
			if (strncmp(reg, "st", 2) == 0)	regtype = mb_status;
			if (strncmp(reg, "ir", 2) == 0)	regtype = mb_input;
			if (strncmp(reg, "hr", 2) == 0)	regtype = mb_holding;
			ok &= regtype != mb_none;
			mb_offset offset;
			ok &= sscanf(reg+2, "%4hx", &offset);

			if (!ok)
				continue;		
	
			json_t *reg_scan_j;
			float reg_scan;
			reg_scan_j = json_object_get(reg_j, "scan");
			if (reg_scan_j)
				reg_scan = json_number_value(reg_scan_j);	

			float scan = 1;
			if (main_scan_j) scan = main_scan;
			if (dev_scan_j) scan = dev_scan;
			if (reg_scan_j) scan = reg_scan;

			string descr, name;

			rid = json_string_value(json_object_get(reg_j, "id"));
			if (!rid)
				rid = reg;

			rname = json_string_value(json_object_get(reg_j, "name"));
			if (!rname)
				rname = rid;

			descr = (string) ifid + "_" + did + "_" + rid;
			name = (string) ifname + " " + dname + " " + rname;

			string unit = "";
			if (json_string_value(json_object_get(reg_j, "unit")))
				unit = json_string_value(json_object_get(reg_j, "unit"));

			mb_datatype datatype = mbd_none;
			if (regtype == mb_coil or regtype == mb_status)
				datatype = mbd_bool;
			if (regtype == mb_input or regtype == mb_holding)
			{
				const char *dt_str = json_string_value(json_object_get(reg_j, "datatype"));
				if (dt_str)
				{
					for (int i = 0; i < mbd_num; i++)
						if (strcmp(mb_datatype_str[i], dt_str) == 0)
							datatype = (mb_datatype) i;
				}
			}
			uint8_t len = mb_datatype_len[datatype];

			in *i;
			out *o;

			if (regtype == mb_coil or regtype == mb_holding)
			{
				if (regtype == mb_coil)
					o = new out(descr, name, unit, 0, (void*) mb, 0, 1, 1);
				if (regtype == mb_holding)
					o = new out(descr, name, unit, 0, (void*) mb, 0, 1, 65535);
				i = o;
			}		
			else
			{
				o = NULL;
				i = new in(descr, name, unit);			
			}

			mb_key key = {id, regtype, offset};
			mb->reg[key].i = i;
			mb->reg[key].o = o;
			mb->reg[key].interval = scan;
			mb->reg[key].key = key;
			mb->reg[key].datatype = datatype;
			mb->reg[key].len = len;
			
		}
	}

}


