#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_mb.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "modbus/modbus.h"
#include "out.h"

//#define debug

using namespace std;

interface_mb::interface_mb(const string d, const string n, const string ipstr):interface(d, n, 1), _ipstr(ipstr), _ctx(NULL)
{
	makeModbusContext(); 	
}

void interface_mb::makeModbusContext()
{
	if (!_ctx)
	{
		_ctx = modbus_new_tcp(_ipstr.c_str(), MODBUS_TCP_DEFAULT_PORT);
		modbus_set_response_timeout(_ctx, 1, 0);
	}
}

interface_mb::~interface_mb()
{
	if (_ctx)
		modbus_free(_ctx);
}

void interface_mb::getIns()
{
/*
	makeModbusContext();

	bool dataOk = false;
	uint8_t incoils[8];
	uint8_t outcoils[8];
	uint16_t regs[48];
	double t = now();
	if(_ctx){
		#ifdef debug
			printf("modbus context is ok\n");
		#endif
		double start = now();
		if (modbus_connect(_ctx) != -1){
			//printf("modbus connect has succes\n");
			if (modbus_read_input_registers(_ctx, 0, 48, regs) == 48 and modbus_read_bits(_ctx, 0, 8, incoils) == 8 and modbus_read_bits(_ctx, 16, 8, outcoils) == 8){
				// alle data is er, set present van alles
				#ifdef debug
					printf("modbus has data\n");
				#endif
				dataOk = true;
				}
			modbus_close(_ctx);
			}
		double end = now();
		t = (start + end) / 2;
		if (dataOk){
			latency->setValue((end-start)*1000, t);
			for (uint16_t i = 0; i<8; i++){
				DI[i]->setValue(incoils[i], t);
				DO[i]->setValue(outcoils[i], t);;
				CI[i]->setValue(regs[2*i] + 65535 * regs[2*i + 1], t);
				}
			}
		}
	if (not _ctx or not dataOk){	// added not dataOK. JCE, 21-8-13
		if (not _ctx) latency->setValid(false); // latency stays valid, just at timeout. JCE, 21-8-13
		for (uint16_t i = 0; i<8; i++){
			DI[i]->setValid(false);
			DO[i]->setValid(false);
			CI[i]->setValid(false); // DI -> CI, JCE, 21-8-13
			}
		}*/
}

// out setter, JCE, 23-7-13
void interface_mb::setOut(out* o, float v)
{
	/*
	if (!globalControl) return;
	makeModbusContext();
	bool newState = true;
	if (v < 0.5) newState = false;
	if(_ctx)
		for (uint16_t i = 0; i<8; i++)
			if (o == DO[i] and newState != DO[i]->getValue())
				if(modbus_connect(_ctx) != -1)
					{
					modbus_write_bit(_ctx, 16+i, newState);
					modbus_close(_ctx); 
					getIns();
					}
	*/
}

void interface_mb::start()
{
	build_outreg();
	build_timereg();
	interface::start();
}

double interface_mb::next_multiple(double val, double interval)
{
	double n = floor(val / interval);
	return interval * (n + 1);
}

void interface_mb::build_outreg()
{
	for( auto id = reg.begin(); id != reg.end(); id++)
		for (auto regtype = id->second.begin(); regtype != id->second.end(); regtype++)
			for( auto offset = regtype->second.begin(); offset != regtype->second.end(); offset++)
				{
					out *o = offset->second.o;
					if (o)
						{
							outreg[o].id = id->first;
							outreg[o].regtype = regtype->first;
							outreg[o].offset = offset->first;
						}
				}

}

void interface_mb::build_timereg()
{
	for( auto id = reg.begin(); id != reg.end(); id++)
		for (auto regtype = id->second.begin(); regtype != id->second.end(); regtype++)
			for( auto offset = regtype->second.begin(); offset != regtype->second.end(); offset++)
			{
				float interval = offset->second.interval;
				double t = next_multiple(now_mt(), interval);
				mb_id _id = id->first;
				mb_regtype _regtype = regtype->first;
				mb_offset _offset = offset->first;
				timereg[t][_id][_regtype][_offset].i = offset->second.i;
				timereg[t][_id][_regtype][_offset].interval = interval;
			}
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

	json_t *main_scan_j;
	float main_scan;
	main_scan_j = json_object_get(json, "scan");
	if (main_scan_j)
		main_scan = json_number_value(main_scan_j);	
	
	interface_mb *mb = new interface_mb(ifid, ifname, ipstr);

	// Reading the individual registers
	// "device":{
	//	"1":{
	//   "hr0004":
	//	  {}, optionally name, id, scan
	const char *device, *reg;
	json_t *device_j, *reg_j;
	json_object_foreach(devices_j, device, device_j)
	{
		json_t *dev_scan_j;
		float dev_scan;
		dev_scan_j = json_object_get(device_j, "scan");
		if (dev_scan_j)
			dev_scan = json_number_value(dev_scan_j);	

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
			if (json_string_value(json_object_get(reg_j, "id")))
				descr = json_string_value(json_object_get(reg_j, "id"));
			else
				descr = (string) ifid + "_" + device + "_" + reg;

			if (json_string_value(json_object_get(reg_j, "name")))
				name = json_string_value(json_object_get(reg_j, "name"));
			else
				name = (string) ifname + " " + device + " " + reg;

			string unit = "";
			if (json_string_value(json_object_get(reg_j, "unit")))
				unit = json_string_value(json_object_get(reg_j, "unit"));

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

			mb->reg[id][regtype][offset].i = i;
			mb->reg[id][regtype][offset].o = o;
			mb->reg[id][regtype][offset].interval = scan;
		}
	}

}


