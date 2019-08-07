#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_solarlog.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "modbus/modbus.h"

//#define DEBUG

using namespace std;

interface_solarlog::interface_solarlog(const string d, const string n, const string ipstr):interface(d, n), _ipstr(ipstr), _ctx(NULL), _lastValidDateTime(0){

	//in *lastUpdateTime, *Pac, *Pdc, *Uac, *Udcavg, *efficiency, *totalKwhProduced, *latency;
	//lastUpdateTime = new in(getDescriptor() + "_lut", getName() + " last update time", "s"); // dat kan niet in een float, helaas. Dat moet in een double!!! en wat is het eingelijk. JCE< 27-6-13
	Pac = new in(getDescriptor() + "_pac", getName() + " Pac", "W");
	Pdc = new in(getDescriptor() + "_pdc", getName() + " Pdc", "W");
	Uac = new in(getDescriptor() + "_uac", getName() + " Uac", "V");
	Udc = new in(getDescriptor() + "_udc", getName() + " Udc avg", "V");
	efficiency = new in(getDescriptor() + "_eff", getName() + " efficiency", "%", 3);
	totalKwhProduced = new in(getDescriptor() + "_tep", getName() + " total energy produced", "kWh", 3);
	latency = new in(getDescriptor() + "_lat", getName() + " latency", "ms", 3);
	//makeModbusContext(); Do not wait for this during startup
	}

interface_solarlog::~interface_solarlog(){
	//delete lastUpdateTime;
	#ifdef DEBUG
		printf("interface_solarlog destroying %s\n", Pac->getName().c_str());
		fflush(stdout);
	#endif
	delete Pac;
	delete Pdc;
	delete Uac;
	delete Udc;
	delete efficiency;
	delete totalKwhProduced;
	delete latency;
	//printf("\nfinished deleting ins\nStart deleting modbus context...\n");
	if (_ctx)
		modbus_free(_ctx);
	//printf("\nfinished deleting modbus context...\n");
	}

void interface_solarlog::makeModbusContext(){
	if (!_ctx){
		//printf("creating modbus context\n");
		_ctx = modbus_new_tcp(_ipstr.c_str(), MODBUS_TCP_DEFAULT_PORT);
		//printf("modbus connect to %s:%i\n", _ipstr.c_str(), MODBUS_TCP_DEFAULT_PORT);
		//struct timeval response_timeout;
		//response_timeout.tv_sec = 1;
		//response_timeout.tv_usec = 0;
		modbus_set_response_timeout(_ctx, 1, 0);
		}
	}

void interface_solarlog::getIns(){
	makeModbusContext();

	bool dataOk = false;
	uint16_t regs[8];
	if(_ctx){
		//printf("modbus context is ok\n");
		double start = now();
		//printf("modbus connecting\n");
		if (modbus_connect(_ctx) != -1){
			//printf("modbus connect has succes\n");
			if (modbus_read_input_registers(_ctx, 3500, 8, regs) == 8){
				// alle data is er, set present van alles
				//printf("modbus has data\n");
				dataOk = true;
				}
			modbus_close(_ctx);
			}
		double end = now();
		double t = (start + end) / 2;
		uint32_t solarlogTime;
		if (dataOk)
			solarlogTime = regs[1] * 65536 + regs[0];
		dataOk = dataOk and solarlogTime % 300 >= 15 and solarlogTime; // lkkr wat een uitzonderingen. JCE, 27-6-13i
		//printf("solarlog's time: %u\n", solarlogTime); 
		//if (!dataOk) printf("the data is not considered valid\n");
		latency->setValue((end-start)*1000, t);
		if (dataOk){
			Pac->setValue(regs[3] * 65536 + regs[2], t);
			Pdc->setValue(regs[5] * 65536 + regs[4], t);
			Uac->setValue(regs[6], t);
			Udc->setValue(regs[7], t);
			if(Pdc->getValue() > 0)
				efficiency->setValue(Pac->getValue() / Pdc->getValue() * 100);
			else
				efficiency->setValid(false);
			if (_lastValidDateTime){
				float dt = t - _lastValidDateTime;
				totalKwhProduced->setValue(totalKwhProduced->getValue() + dt * Pdc->getValue() / (1000*3600));
				}
			_lastValidDateTime = t;
			}
		}
	else
		latency->setValid(false);

	if(not dataOk){
		Pac->setValid(false);
		Pdc->setValid(false);
		Uac->setValid(false);
		Udc->setValid(false);
		Udc->setValid(false);
		efficiency->setValid(false);
		}
	}

