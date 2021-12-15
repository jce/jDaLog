#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_adam6052.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "modbus/modbus.h"
#include "out.h"

//#define debug

using namespace std;

interface_adam6052::interface_adam6052(const string d, const string n, float i, const string ipstr):interface(d, n, i), _ipstr(ipstr), _ctx(NULL), _lastValidDateTime(0){
	latency = new in(getDescriptor() + "_lat", getName() + " latency", "ms", 3);
	char name[100], descr[100];
	for (uint16_t i = 0; i<8; i++){
		snprintf(descr, 100, "_DI%u", i);
		snprintf(name, 100, " DI%u", i);
		DI[i] = new in(getDescriptor() + descr, getName() + name, "");
		snprintf(descr, 100, "_DO%u", i);
		snprintf(name, 100, " DO%u", i);
		DO[i] = new out(getDescriptor() + descr, getName() + name, "", 0, this);
		snprintf(descr, 100, "_CI%u", i);
		snprintf(name, 100, " Counter %u", i);
		CI[i] = new in(getDescriptor() + descr, getName() + name, "");
		}
	makeModbusContext(); 	
	}

void interface_adam6052::makeModbusContext(){
	if (!_ctx){
		_ctx = modbus_new_tcp(_ipstr.c_str(), MODBUS_TCP_DEFAULT_PORT);
		#ifdef debug
			printf("modbus connect to %s:%i\n", _ipstr.c_str(), MODBUS_TCP_DEFAULT_PORT);
		#endif
		//struct timeval response_timeout;
		//response_timeout.tv_sec = 1;
		//response_timeout.tv_usec = 0;
		//modbus_set_response_timeout(_ctx, &response_timeout);
		modbus_set_response_timeout(_ctx, 1, 0);
		}
	}

interface_adam6052::~interface_adam6052(){
	for (uint16_t i = 0; i<8; i++){
		delete DI[i];
		delete DO[i];
		delete CI[i];
		}
	delete latency;
	if (_ctx)
		modbus_free(_ctx);
	}

void interface_adam6052::getIns(){
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
	}

// out setter, JCE, 23-7-13
void interface_adam6052::setOut(out* o, float v){
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
	}
