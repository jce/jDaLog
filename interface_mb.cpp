#include "endian.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_mb.h"
#include "main.h"
#include "tcmath.h"
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
const char* mb_datatype_str[mbd_num] = {"none", "bool", "uint16", "int16", "uint32r", "int32r", "uint32", "int32", "uint64", "int64", "float16", "float32", "float64"};
const uint8_t mb_datatype_len[mbd_num] = { 1,	1,		1,		  1,		2,		  2,        2,        2,       4,        4,       1,         2,         4};
void printsch(reg_context **);

interface_mb::interface_mb(const string d, const string n, const string ipstr, uint16_t _port):interface(d, n, 1), 
	_ipstr(ipstr), port(_port), comtype(mbc_tcp)
{
}

interface_mb::interface_mb(const string d, const string n, string _device, int _baud, char _parity, int _data_bit, int _stop_bit):interface(d, n, 1), 
	device(_device), baud(_baud), data_bit(_data_bit), stop_bit(_stop_bit), parity(_parity), comtype(mbc_rtu)
{
}

interface_mb::~interface_mb()
{
}

void interface_mb::setOut(out*, float)
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

#define PRINT_ERROR()\
{	\
	if (comtype == mbc_tcp) \
		printf("Modbus connection to %s:%d failed. Errno %d: %s\n", _ipstr.c_str(), port, errno, modbus_strerror(errno)); \
	if (comtype == mbc_rtu) \
		printf("Modbus connection to %s %d%c%d %d Baud failed. Errno %d: %s\n", device.c_str(), data_bit, parity, stop_bit, baud, errno, modbus_strerror(errno)); \
}

#define PRINT_RESTORED() \
{ \
	if (comtype == mbc_tcp) \
		printf("Modbus connection to %s:%d restored.\n", _ipstr.c_str(), port); \
	if (comtype == mbc_rtu) \
		printf("Modbus connection to %s %d%c%d %d Baud Restored.\n", device.c_str(), data_bit, parity, stop_bit, baud); \
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
	modbus_t *ctx;
	if (comtype == mbc_tcp)
		ctx = modbus_new_tcp(_ipstr.c_str(), port);
	if (comtype == mbc_rtu)
		ctx = modbus_new_rtu(device.c_str(), baud, parity, data_bit, stop_bit);
	// Too bad, the library does not have modbus ascii.

	modbus_set_response_timeout(ctx, 1, 0);
	bool conmem = 1;	// To display errors only once.
	run_flg = true;
	
	// Initial connection loop. Try every second.
	while (run_flg && modbus_connect(ctx) != 0)
	{
		if (conmem)
		{
			PRINT_ERROR();
			conmem = 0;
		}
		sleep(1);
	}

	if (!conmem)
	{
		PRINT_RESTORED();
		conmem = 1;
	}

	while(run_flg)
	{
		t = now_mt();
		start = schedule;
		end = schedule;
		num = start->len;

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
		while (start->adj_before && t > start->adj_before->time && num + start->adj_before->len <= max_fetch)
		{
			start = start->adj_before;
			num += start->len;
		}
		// Find the last item
		while ( end->adj_after && t > end->adj_after->time && num + end->adj_after->len <= max_fetch)
		{
			end = end->adj_after;
			num += end->len;
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
		//printsch(&schedule);

		// To in.
		reg = start;
		if (rv == num)
		{
			num = 0;	// Is now offset in received message.
			t = now();
			if (key.regtype == mb_coil || key.regtype == mb_status)
				while (true)
				{
					reg->i->setValue(bits[num], t);
					num += reg->len;
					if (reg == end)
						break;
					reg = reg->adj_after;
				}
			if (key.regtype == mb_input || key.regtype == mb_holding)
				while (true)
				{
					//reg->i->setValue(regs[num], t);
					reg->i->setValue(reg->readconv(& regs[num]), t);
					num += reg->len;
					if (reg == end)
						break;
					reg = reg->adj_after;
				}
			if (!conmem)
			{
				PRINT_RESTORED();
				conmem = 1;
			}
		}
		else
		{
			while (true)
			{
				reg->i->setValid(false);
				if (reg == end)
					break;
				reg = reg->adj_after;
			}
			if (conmem)
			{
				PRINT_ERROR();
				conmem = 0;
			}
		}

		// There is a failure mode for modbus rtu, in which the usb rs485 adapter is temporarily disconnected.
		// Libmodbus' auto reconnect does not catch this.
		if (conmem == 0 && comtype == mbc_rtu)
		{
			modbus_close(ctx);
			while (run_flg && ! modbus_connect(ctx) == 0 )
			{
				modbus_close(ctx);
				sleep(1);
			}
			conmem = 1;
			PRINT_RESTORED();
		}
				
		// Reschedule
		reg = start;
		t = now_mt();
		while (true)
		{
			reg->time = next_multiple(t, reg->interval);
			reschedule(reg);
			if (reg == end)
				break;
			reg = reg->adj_after;
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
		if ( (l->first.id == r->first.id) && (l->first.regtype == r->first.regtype) && (l->first.offset + l->second.len == r->first.offset) )
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
		printf("%p -> %p	%f	%d.%d.%d\n", *x, (*x)->next, (*x)->time, (*x)->key.id, (*x)->key.regtype, (*x)->key.offset);
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

// Dont get scared now, here follows some casting and conversion magic.
double read_none(uint16_t*)				{ return 0; };
double read_uint16(uint16_t *m)			{ return *m; };
double read_int16(uint16_t *m)			{ return *((int16_t*) &m); };
double read_uint32(uint16_t *m)			{ uint32_t x = (((uint32_t) (*m)) << 16) + (* (m+1)); return x; };
double read_int32(uint16_t *m)			{ uint32_t x = (((uint32_t) (*m)) << 16) + (* (m+1)); return * (int32_t*) &x; };
double read_uint32r(uint16_t *m)		{ uint32_t x = (((uint32_t) (*(m+1))) << 16) + (* m); return x; };
double read_int32r(uint16_t *m)			{ uint32_t x = (((uint32_t) (*(m+1))) << 16) + (* m); return * (int32_t*) &x; };
double read_uint64(uint16_t *m)			{ uint64_t x = ((uint64_t) (*m) << 48) + ((uint64_t) (* m+1) << 32) + ((uint32_t) (* m+2) << 16) + (* m+3); return x; };
double read_int64(uint16_t *m)			{ uint64_t x = ((uint64_t) (*m) << 48) + ((uint64_t) (* m+1) << 32) + ((uint32_t) (* m+2) << 16) + (* m+3); return * (int64_t*) x; };
double read_float16(uint16_t *m)		{ return float16_to_double((float16_t) *m); }; 
double read_float32(uint16_t *m)		{ uint32_t x = ((uint32_t) (*m) << 16) + (* m+1); return * (float*) &x; };
double read_float64(uint16_t *m)		{ uint64_t x = ((uint64_t) (*m) << 48) + ((uint64_t) (* m+1) << 32) + ((uint32_t) (* m+2) << 16) + (* m+3); return * (double*) x; };

// Todo: big endian translation should be left to the modbus library. To be removed. JCE, 9-1-2020
void write_none(double, uint16_t*)			{ return; }
void write_uint16(double d, uint16_t *m)	{ *m = htobe16(d); };
void write_int16(double d, uint16_t *m)		{ int16_t x = d; *m = htobe16(* (uint16_t*) &x ); };
void write_uint32(double d, uint16_t *m)	{ *m = htobe32(d); };
void write_int32(double d, uint16_t *m)		{ int32_t x = d; *m = htobe32(* (uint32_t*) &x); };
void write_uint32r(double d, uint16_t *m)	{ *m = htobe32(d); }; //todo
void write_int32r(double d, uint16_t *m)	{ int32_t x = d; *m = htobe32(* (uint32_t*) &x); }; // todo
void write_uint64(double d, uint16_t *m)	{ *m = htobe64(d); };
void write_int64(double d, uint16_t *m)		{ int64_t x = d; *m = htobe64(* (uint64_t*) &x); };
void write_float16(double d, uint16_t *m)	{ *m = htobe16( double_to_float16(d) ); }
void write_float32(double d, uint16_t *m)	{ uint32_t x = * (uint32_t*) &d; * (uint32_t*) m = htobe32(x); };
void write_float64(double d, uint16_t *m)	{ uint64_t x = * (uint64_t*) &d; * (uint64_t*) m = htobe64(x); };

// Factory for interface_mb.
void interface_mb_from_json(const char *ifid, const char *ifname, json_t *json)
{
	if (!ifid)
	{
		printf("interface_mb: no id given.\n");
		return;
	}	
	json_t *devices_j = json_object_get(json, "device");
	if (!devices_j)
	{
		printf("interface_mb: no devices given.\n");
		return;
	}

	json_t *main_scan_j;
	float main_scan;
	main_scan_j = json_object_get(json, "scan");
	if (main_scan_j)
		main_scan = json_number_value(main_scan_j);	

	const char *typestr = json_string_value(json_object_get(json, "type"));
	const char *ipstr = json_string_value(json_object_get(json, "address"));
	const char *fstr = json_string_value(json_object_get(json, "file"));
	interface_mb *mb;

	if (!ifname)
		ifname = ifid;
	
	// TCP
	if ((strcmp(typestr, "mb_tcp") == 0) || (ipstr && !fstr) )
	{
		if (!ipstr)
		{
			printf("interface_mb: no address given.\n");
			return;
		}

		uint16_t port = MODBUS_TCP_DEFAULT_PORT;
		json_t *jport = json_object_get(json, "port");
		if (json_is_integer(jport))
			port = json_integer_value(jport);
	
		mb = new interface_mb(ifid, ifname, ipstr, port);
	}
	// RTU
	else if ((strcmp(typestr, "mb_rtu") == 0) || (!ipstr && fstr) )
	{
		if (!fstr)
		{
			printf("interface_mb: no device given.\n");
			return;
		}

		int baud = 9600;
		char parity = 'N';
		int data_bit = 8;
		int stop_bit = 1;
		json_t *jbaud  = json_object_get(json, "baud");
		json_t *jparity  = json_object_get(json, "parity");
		json_t *jdata_bit  = json_object_get(json, "data_bit");
		json_t *jstop_bit  = json_object_get(json, "stop_bit");
		if (json_is_integer(jbaud))
			baud = json_integer_value(jbaud);
		if (json_is_string(jparity))
			parity = json_string_value(jparity)[0];
		if (json_is_integer(jdata_bit))
			data_bit = json_integer_value(jdata_bit);
		if (json_is_integer(jstop_bit))
			stop_bit = json_integer_value(jstop_bit);
	
		mb = new interface_mb(ifid, ifname, fstr, baud, parity, data_bit, stop_bit);
	}
	else
	{
		printf("interface_mb: modbus type (tcp or rtu) could not be determined\n");
		return;
	}

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
				datatype = mbd_uint16;
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
			reg_context *reg = &mb->reg[key];
			reg->i = i;
			reg->o = o;
			reg->interval = scan;
			reg->key = key;
			reg->datatype = datatype;
			reg->len = len;
			switch(datatype)
			{
				case mbd_none:
				case mbd_bool:
				case mbd_num:
					reg->readconv = read_none;
					reg->writeconv = write_none;
					break;
				case mbd_uint16:
					reg->readconv = read_uint16;
					reg->writeconv = write_uint16;
					break;
				case mbd_int16:
					reg->readconv = read_int16;
					reg->writeconv = write_int16;
					break;
				case mbd_uint32:
					reg->readconv = read_uint32;
					reg->writeconv = write_uint32;
					break;
				case mbd_int32:
					reg->readconv = read_int32;
					reg->writeconv = write_int32;
					break;
				case mbd_uint32r:
					reg->readconv = read_uint32r;
					reg->writeconv = write_uint32r;
					break;
				case mbd_int32r:
					reg->readconv = read_int32r;
					reg->writeconv = write_int32r;
					break;
				case mbd_uint64:
					reg->readconv = read_uint64;
					reg->writeconv = write_uint64;
					break;
				case mbd_int64:
					reg->readconv = read_int64;
					reg->writeconv = write_int64;
					break;
				case mbd_float16:
					reg->readconv = read_float16;
					reg->writeconv = write_float16;
					break;
				case mbd_float32:
					reg->readconv = read_float32;
					reg->writeconv = write_float32;
					break;
				case mbd_float64:
					reg->readconv = read_float64;
					reg->writeconv = write_float64;
					break;
			}
		}
	}
}


