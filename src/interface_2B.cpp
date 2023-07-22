//#define byte byte_override
//#include "snap7.h"
//#undef byte

#include "interface_2B.h"
#include "interface_S1200.h"
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

#define DBG(...)
//#define DBG(...) { printf(__VA_ARGS__); printf("\n"); }

#define DB_NR		8
#define READ_SIZE	32
#define RUNTIME_MIN	2
// type( offset_in_db, descriptor_addition_string, public_name_addition_string, unit_string, decimals, factor )
#define READVALUES \
	I32(	0,		"rt",	"runtime",						"s",		0,	1)\
	BOOL(	4,	0, 	"bs",	"beweging slaapkamer")\
	BOOL(	4,	1, 	"bb",	"beweging badkamer")\
	BOOL(	4,	2, 	"bka",	"beweging kamer")\
	BOOL(	4,	3, 	"bke",	"beweging keuken")\
	BOOL(	4,	4, 	"bh",	"beweging hal")\
	BOOL(	4,	5, 	"dw",	"deurbel Wilgenlaan 2")\
	BOOL(	4,	6, 	"kkw",	"knopje keuken Wilgenlaan 2")\
	BOOL(	4,	7, 	"tf",	"toilet flush")\
	BOOL(	5,	0, 	"ls",	"licht slaapkamer")\
	BOOL(	5,	1, 	"lb",	"licht badkamer")\
	BOOL(	5,	2, 	"lka",	"licht kamer")\
	BOOL(	5,	3, 	"lke",	"licht keuken")\
	BOOL(	5,	4, 	"lh",	"licht hal")\
	BOOL(	5,	5, 	"wp",	"wifi power")\
	BOOL(	5,	6, 	"cp",	"circulatie pump power")\
	BOOL(	5,	7, 	"dbu",	"deur buiten")\
	BOOL(	6,	0, 	"dh",	"deur hal")\
	BOOL(	6,	1, 	"dk",	"deur keuken")\
	BOOL(	6,	2, 	"ds",	"deur slaapkamer")\
	BOOL(	6,	3, 	"dba",	"deur badkamer")\
	I16(	14,		"bt", 	"temperatuur buiten",	 		"degC", 	2, 	0.01) \
	I16(	16,		"kmt", 	"temperatuur ketel midden", 	"degC", 	2, 	0.01) \
	I16(	18,		"kbt", 	"temperatuur ketel boven",	 	"degC", 	2, 	0.01) \
	I32(	24,		"wm", 	"watermeter",	 				"L", 		1, 	0.5) \
// READVALUES

using namespace std;

interface_2B::interface_2B(const string d, const string n, float i, const string ipstr):interface(d, n, i), _ipstr(ipstr)
{
    PLC = Cli_Create();
    Cli_SetConnectionType(PLC, CONNTYPE_PG);// PG, PC or BASIC
    Cli_ConnectTo(PLC, ipstr.c_str(), 0, 2);

	ins["latency"] = new in(getDescriptor() + "_lt", getName() + " latency", "ms", 3);

	#define BOOL(_DBOFFSET_, _BITOFFSET_, _DESCR_, _NAME_) \
		ins[_DESCR_] = new in(getDescriptor() + "_" + _DESCR_, getName() + " " + _NAME_);
	#define I16(_DBOFFSET_, _DESCR_, _NAME_, _UNIT_, _DECIMALS_, _FACTOR_) \
		ins[_DESCR_] = new in(getDescriptor() + "_" + _DESCR_, getName() + " " + _NAME_, _UNIT_, _DECIMALS_);
	#define I32(_DBOFFSET_, _DESCR_, _NAME_, _UNIT_, _DECIMALS_, _FACTOR_) \
		ins[_DESCR_] = new in(getDescriptor() + "_" + _DESCR_, getName() + " " + _NAME_, _UNIT_, _DECIMALS_);
	READVALUES	
	#undef I32
	#undef I16
	#undef BOOL
}

interface_2B::~interface_2B()
{
	Cli_Disconnect(PLC);
	for (auto i = ins.begin(); i != ins.end(); i++)
		delete i->second;
}

void interface_2B::getIns()
{
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

		// offset 16
		//DBG("%i", (int16_t) be16toh(*(uint16_t*) (data+16)));

		//ins["temp_buiten"]->setValue( (float) be16toh(*(uint16_t*) (data+2)) / 100, t);
		//ins["temp_ketel"]->setValue(  (float) be16toh(*(uint16_t*) (data+4)) / 100, t);
	/*
		// JCE, 30-4-2019: Added uptime hack, many measurements start as "0" adding erronous results to the logdata.
		memcpy(&uptime, data + 72, 4);
		uptime = be32toh(uptime);
		//uptime_in->setValue(uptime, t);		
		
			memcpy(&f, data + 22, 4);
			f = beftoh(f);
			hyfpressure->setValue(f, t);
	
			memcpy(&u, data + 10, 4);
			u = be32toh(u);
			hyfstarts->setValue(u, t);
			
			memcpy(&d, data + 14, 8);
			d = bedtoh(d);
			hyfruntime->setValue(d, t);
			
				b_roomMotionTop = data[34] & (1 << 1);
				roomMotionTop->setValue(b_roomMotionTop, t);
	
		}
	*/
	}

	// Prepare the data area to be accepted by the PLC
	/*
	memset(data, 0, 114);
	writecounter++;
	u = htobe32(writecounter);
	memcpy(data+6, &u, 4);
	memcpy(data+110, &u, 4);
	u = htobe32(MAGICNR_WRITE);
	memcpy(data, &u, 4);
	version = htobe16(VERSION_WRITE);
	memcpy(data+4, &version, 2);
	// Fill in the data to transfer

	in *rwl_wv = get_in("rwl_wv");
	in *xp_mp = get_in("xp_mp");
	if (rwl_wv && xp_mp)
	{
		f = rwl_wv->getValue();
		f = htobef(f);
		memcpy(data+10, &f, 4);
		f = xp_mp->getValue();
		if (f>0.5) data[14] |= (1 << 0);
		if (rwl_wv->isValid()) data[14] |= (1 << 1);	
 		if (xp_mp->isValid()) data[14] |= (1 << 2);	
		//if (Q0_0->getValue()) data[14] |= (1 << 3);

		// Write to PLC
		rv = Cli_DBWrite(PLC, 86, 0, 114, &data);
	}

	// Close the connection to the PLC. JCE, 15-10-2016
	//Cli_Disconnect(PLC);
	*/
	}

void interface_2B::setOut(out* o, float v)
{
	o->setValue(v);
}
