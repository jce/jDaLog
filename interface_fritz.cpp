#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_host.h"	// system_exec()
#include "interface_fritz.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "modbus/modbus.h"
#include "out.h"
#include "timefunc.h"

//#define DBG(...) { printf(__VA_ARGS__); printf("\n"); }
#define DBG(...)

using namespace std;
#define DT 6

interface_fritz::interface_fritz(const string d, const string n):interface(d, n, 0.2)
{
	ul = new in("fritz_ul", "fritz upload", "bps", 0);
	dl = new in("fritz_dl", "fritz download", "bps", 0);
}

interface_fritz::~interface_fritz()
{
	delete ul;
	delete dl;
}

void interface_fritz::getIns()
{
	// # https://blog.devnu11.net/2015/02/monitoring-a-fritzbox-via-upnp-fritzos-6-20-fritzbox-7490/ 
	string response = system_exec("curl \"http://fritz.box:49000/igdupnp/control/WANCommonIFC1\" -H \"Content-Type: text/xml; charset=\"utf-8\"\" -H \"SoapAction:urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1#GetAddonInfos\" -d \"<?xml version='1.0' encoding='utf-8'?> <s:Envelope s:encodingStyle='http://schemas.xmlsoap.org/soap/encoding/' xmlns:s='http://schemas.xmlsoap.org/soap/envelope/'> <s:Body> <u:GetAddonInfos xmlns:u='urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1' /> </s:Body> </s:Envelope>\" 2>/dev/null");
	DBG(response.c_str());

	double t = now();
	unsigned long long txrate, rxrate;
	bool ok = 1;
	// It seems the tx and rx counters sometimes rewind a little? Really strange. And this invalidates send and
	// receive counters for derivating a send and receive rate.
	//ok &= findUnsignedLongLongAfter(response.c_str(), "<NewX_AVM_DE_TotalBytesSent64>", tx);
	//ok &= findUnsignedLongLongAfter(response.c_str(), "<NewX_AVM_DE_TotalBytesReceived64>", rx);
	ok &= findUnsignedLongLongAfter(response.c_str(), "<NewByteSendRate>", txrate);
	ok &= findUnsignedLongLongAfter(response.c_str(), "<NewByteReceiveRate>", rxrate);
	if (ok && (( txrate != txrate_prev) || (rxrate != rxrate_prev)))
	{
		ul->setValue(txrate, t);
		dl->setValue(rxrate, t);
		DBG("fritz t: %lf, dt:%lf, txrate: %llu, rxrate: %llu", t, (double) DT, txrate, rxrate);
		txrate_prev = txrate;
		rxrate_prev = rxrate;
	}
}


void interface_fritz_from_json(const char*, const char*, json_t*)
{
	static interface_fritz *fritz = NULL;
	if (!fritz)
		fritz = new interface_fritz("fritz", "fritz");
}
