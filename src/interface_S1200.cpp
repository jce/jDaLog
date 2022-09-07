//#define byte byte_override
//#include "snap7.h"
//#undef byte

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

//#define debug
#define MAGICNR 3245762236
#define MAGICNR_WRITE 2530095196
#define VERSION_WRITE 1

using namespace std;

interface_S1200::interface_S1200(const string d, const string n, float i, const string ipstr):interface(d, n, i), _ipstr(ipstr)
{
	latency = new in(getDescriptor() + "_lt", getName() + " latency", "ms", 3);
	//S1200 = new TS7Client();
	//Cli_Create();

       // S7Object PLC;
        PLC = Cli_Create();
        Cli_SetConnectionType(PLC, CONNTYPE_PG);// PG, PC or BASIC
        //char data[65536];
        Cli_ConnectTo(PLC, ipstr.c_str(), 0, 1);
        //printf(" %X\n", Cli_ConnectTo(PLC, ipstr.c_str(), 0, 1));
        //printf(" %X\n", Cli_ConnectTo(PLC, "10.0.1.10", 0, 1));
       // printf(" %X\n", Cli_DBRead(PLC, 85, 0, sizeof(DBHeader), &data));

	hyfstarts = new in(getDescriptor() + "_hs", getName() + " hydrofoor starts", "", 0);
	hyfruntime = new in(getDescriptor() + "_hr", getName() + " hydrofoor runtime", "s", 1);
	hyfpressure = new in(getDescriptor() + "_hp", getName() + " hydrofoor pressure", "Bar g", 3);
	hyflont = new in(getDescriptor() + "_hlont", getName() + " hydrofoor last on time", "s", 1);
	hyflofft = new in(getDescriptor() + "_hlofft", getName() + " hydrofoor last off time", "s", 1);

	roomIOOnline = new in(getDescriptor() + "_rioo", getName() + " room IO online", "", 0);
	roomMotionTop = new in(getDescriptor() + "_rmt", getName() + " room motion top", "", 0);
	roomMotionDesk = new in(getDescriptor() + "_rmd", getName() + " room motion desk", "", 0);
	roomDoorOpen = new in(getDescriptor() + "_rdo", getName() + " room door open", "", 0);
	roomMotionCorridor = new in(getDescriptor() + "_rmc", getName() + " room motion corridor", "", 0);
	roomPresence = new in(getDescriptor() + "_rp", getName() + " room presence estimation", "", 0);
	roomLight = new in(getDescriptor() + "_rl", getName() + " room light setpoint", "", 4);
	roomTemp = new in(getDescriptor() + "_rt", getName() + " room temperature", "degC", 3);
	roomRH = new in(getDescriptor() + "_rh", getName() + " room humidity", "% RH", 3);
	chickenDeskMotion = new in(getDescriptor() + "_cdm", getName() + " chicken desk motion", "", 0);
	
	chickenGatePosEst = new in(getDescriptor() + "_cgpe", getName() + " chicken gate position", "", 0);
	rainDetector = new in(getDescriptor() + "_rain", getName() + " rain detector", "", 0);
	scantime = new in(getDescriptor() + "_st", getName() + " scantime", "ms", 2);

	roomLearnedSleep = new in(getDescriptor() + "_rlsl", getName() + " room learned sleep", "", 3);
	roomLearnedPresence = new in(getDescriptor() + "_rlp", getName() + " room learned presence", "", 3);
	roomWindowFB = new in(getDescriptor() + "_rwp", getName() + " room window position", "", 4);
	roomSleepSw = new in(getDescriptor() + "rsls", getName() + " room sleep switch", "", 0);
	roomComp = new in(getDescriptor() + "_rc", getName() + " room computer", "", 0);
	fishTankPump = new in(getDescriptor() + "_ftp", getName() + " fish tank pump", "", 0);


	sheepWater = new in(getDescriptor() + "_shw", getName() + " Sheep water cumulative", "L", 2);
	sheepFoodTime = new in(getDescriptor() + "_shf", getName() + " Sheep at food time cumulative", "s", 0);
	sheepWaterPulse = new in(getDescriptor() + "_shwp", getName() + " Sheep water abs pulses", "l/2min", 0);
	sheepFoodPresence = new in(getDescriptor() + "_shfp", getName() + " Sheep at food presence", "", 0);

	uptime_in = new in(getDescriptor() + "_upt", getName() + " Uptime", "s", 0);


	rain_bucket_tips = new in(getDescriptor() + "_rbt", getName() + " Rain bucket tips", "", 0);
	rain_count = new in(getDescriptor() + "_rac", getName() + " Rain total count", "mm", 1);

	room_CO = new in(getDescriptor() + "_rco", getName() + " room CO", "PPM", 1);
	room_smoke_temp = new in(getDescriptor() + "_rst", getName() + " room smoke temperature", "degC", 1);

	rain_rate = new in(getDescriptor() + "_rr", getName() + " rain rate", "mm/h", 2);

	water_tank_level = new in(getDescriptor() + "_wtl", getName() + " water tank level", "m", 3);
	water_tank_volume = new in(getDescriptor() + "_wtv", getName() + " water tank volume", "m3", 3);

	Bed_B1 = new in(getDescriptor() + "_b1", getName() + " bed 1", "Ohm", 3);
	Bed_B2 = new in(getDescriptor() + "_b2", getName() + " bed 2", "Ohm", 3);
	Bed_B3 = new in(getDescriptor() + "_b3", getName() + " bed 3", "Ohm", 3);
	Bed_B4 = new in(getDescriptor() + "_b4", getName() + " bed 4", "Ohm", 3);

//	Q0_0 = new out(getDescriptor() + "_Q0_0", getName() + " Q0.0", "", 0, (void*)this);

	writecounter = 0;
}

interface_S1200::~interface_S1200(){
	Cli_Disconnect(PLC);

	delete Bed_B4;
	delete Bed_B3;
	delete Bed_B2;
	delete Bed_B1;

	delete water_tank_volume;
	delete water_tank_level;

	delete rain_rate;

	delete room_smoke_temp;
	delete room_CO;

	delete rain_count; 
	delete rain_bucket_tips;

	delete uptime_in;

	delete sheepFoodPresence;
	delete sheepWaterPulse;
	delete sheepWater;
	delete sheepFoodTime;
	delete fishTankPump;
	delete roomComp;
	delete roomSleepSw;
  	delete roomWindowFB;
	delete roomLearnedPresence;
   	delete roomLearnedSleep;

	delete scantime;
	delete rainDetector;
	delete chickenGatePosEst;
	delete chickenDeskMotion;
	delete roomTemp;
	delete roomRH;
	delete roomLight;
	delete roomPresence;
	delete roomMotionCorridor;
	delete roomDoorOpen;
	delete roomMotionDesk;
	delete roomMotionTop;
	delete roomIOOnline;
	delete hyflofft;
	delete hyflont;
	delete hyfpressure;
	delete hyfruntime;
	delete hyfstarts;
	delete latency;
	}


float htobef(float f){
	uint32_t in = 0, out = 0;
	float out_f = 0;
	memcpy(&in, &f, 4);
	out = htobe32(in);
	memcpy(&out_f, &out, 4);
	return out_f;
	}

float beftoh(float f){
	uint32_t in = 0, out = 0;
	float out_f = 0;
	//in = *((uint32_t*) &f);
	memcpy(&in, &f, 4);
	out = be32toh(in);
	//out_f = *((float*) &out);
	memcpy(&out_f, &out, 4);
	return out_f;
	}

double bedtoh(double d){
	uint64_t in = 0, out = 0;
	double out_f = 0;
	//in = *((uint32_t*) &f);
	memcpy(&in, &d, 8);
	out = be64toh(in);
	//out_f = *((float*) &out);
	memcpy(&out_f, &out, 8);
	return out_f;
	}

void interface_S1200::getIns()
{

	interaction_counter++;
	interaction_30 = interaction_counter == 30;
	if (interaction_30)
		interaction_counter = 0;

	char data[65535];
	double d = 0;
	float f = 0;
	uint32_t rv, u = 0, scanCA = 0, scanCB = 0, magicNr = 0, uptime = 0, rain_ticks = 0;
	uint16_t version = 0;
	
	// Read part
	double start = now();
    rv = Cli_DBRead(PLC, 85, 0, 164, &data);
	if (rv != 0) {
		Cli_Disconnect(PLC);
		Cli_ConnectTo(PLC, _ipstr.c_str(), 0, 1);
        	rv = Cli_DBRead(PLC, 85, 0, 164, &data);
		}
	double end = now();

	double t = (start + end) / 2;
	bool b_roomIOOnline = false, b_roomMotionTop, b_roomMotionDesk, b_roomDoorOpen, b_roomMotionCorridor, b_roomPresence;
	bool rainSensorOK = false, roomTemperatureReadoutOK = false;
	bool b_roomComp = false, b_roomSleepSw = false;
	bool b_fishTankPump; // JCE, 30-8-2018
	latency->setValue((end-start)/1*1000, t); // 1 request, scale = ms
	if(rv == 0) {

		memcpy(&magicNr, data + 0, 4);
		magicNr = be32toh(magicNr);
		memcpy(&version, data + 4, 2);
		version = be16toh(version);
		memcpy(&scanCA, data + 6, 4);
		scanCA = be32toh(scanCA);
		memcpy(&scanCB, data + 160, 4);
		scanCB = be32toh(scanCB);

		// JCE, 30-4-2019: Added uptime hack, many measurements start as "0" adding erronous results to the logdata.
		memcpy(&uptime, data + 72, 4);
		uptime = be32toh(uptime);
		uptime_in->setValue(uptime, t);		

		// Added, JCE, 4-5-2022
		memcpy(&f, data + 92, 4);
		f = beftoh(f);
		rain_rate->setValue(f, t);

		// Added, JCE, 8-7-2022
		if (interaction_30)
		{		
			memcpy(&f, data + 96, 4);
			f = beftoh(f);
			water_tank_level->setValue(f, t);

			memcpy(&f, data + 100, 4);
			f = beftoh(f);
			water_tank_volume->setValue(f, t);
		}	

		if (rv == 0 and scanCA == scanCB and magicNr == MAGICNR and version == 1 and scancounter != scanCA and uptime > 10)
		{
			if (lastReadTime > 1 and (scanCA - scancounter) != 0)
			{
				f = 1000.0 * (t-lastReadTime) / (scanCA - scancounter);
				if ((f - lastScanTime) < (0.2 * lastScanTime) and
					(f-lastScanTime) > (-0.2 * lastScanTime)) 
					scantime->setValue(f, t);
				lastScanTime = f;
			}
			scancounter = scanCA;
			lastReadTime = t;
		
			memcpy(&f, data + 22, 4);
			f = beftoh(f);
			hyfpressure->setValue(f, t);
	
			memcpy(&u, data + 10, 4);
			u = be32toh(u);
			hyfstarts->setValue(u, t);
			
			memcpy(&d, data + 14, 8);
			d = bedtoh(d);
			hyfruntime->setValue(d, t);
			
			memcpy(&f, data + 26, 4);
			f = beftoh(f);
			hyflont->setValue(f, t);

			memcpy(&f, data + 30, 4);
			f = beftoh(f);
			hyflofft->setValue(f, t);




			b_roomIOOnline = data[34] & (1 << 0);
			roomIOOnline->setValue(b_roomIOOnline, t);

			chickenDeskMotion->setValue(bool(data[34] & (1 << 7)), t);
			chickenGatePosEst->setValue((uint8_t) data[48],t);
			
			b_fishTankPump = data[49] & (1 << 7);
			fishTankPump->setValue(b_fishTankPump, t);

			if (b_roomIOOnline)
			{
				b_roomMotionTop = data[34] & (1 << 1);
				b_roomMotionDesk = data[34] & (1 << 2);
				b_roomMotionCorridor = data[34] & (1 << 3);
				b_roomDoorOpen = data[34] & (1 << 4);
				b_roomPresence = data[34] & (1 << 5);
				b_roomComp = data[49] & (1 << 5);
				b_roomSleepSw = data[49] & (1 << 6);
				
				memcpy(&f, data + 44, 4);
				f = beftoh(f);
				roomLight->setValue(f, t);
			}

			roomMotionTop->setValue(b_roomMotionTop, t);
			roomMotionDesk->setValue(b_roomMotionDesk, t);
			roomMotionCorridor->setValue(b_roomMotionCorridor, t);
			roomDoorOpen->setValue(b_roomDoorOpen, t);
			roomPresence->setValue(b_roomPresence, t);
			roomComp->setValue(b_roomComp, t);
			roomSleepSw->setValue(b_roomSleepSw, t);

			memcpy(&f, data + 50, 4);
			f = beftoh(f);
			roomWindowFB->setValue(f, t);	

			memcpy(&f, data + 54, 4);
			f = beftoh(f);
			roomLearnedPresence->setValue(f, t);	

			memcpy(&f, data + 58, 4);
			f = beftoh(f);
			roomLearnedSleep->setValue(f, t);	

			roomTemperatureReadoutOK = data[49] & (1 << 0); 
			if (roomTemperatureReadoutOK)
			{
				memcpy(&f, data + 36, 4);
				f = beftoh(f);
				roomTemp->setValue(f, t);

				memcpy(&f, data + 40, 4);
				f = beftoh(f);
				roomRH->setValue(f, t);	
			}

			rainSensorOK = data[49] & (1 << 2);
			if (rainSensorOK)
				rainDetector->setValue((data[49] & (1 << 1))?1:0, t); // This is called an inline if-statement. JCE, 16-7-2016
		
			memcpy(&f, data + 62, 4);
			f = beftoh(f);
			if (f > 0) // Hack to eliminate zeroes from the data. JCE, 30-4-2019
				sheepWater->setValue(f, t);

			memcpy(&f, data +66, 4);
			f = beftoh(f);
			if (f > 0) // Hack to eliminate zeroes from the data. JCE, 30-4-2019
				sheepFoodTime->setValue(f, t);

			// Added, JCE, 22-9-2018
			//	in *sheepWaterPulse, *sheepFoodPresence; // Abstracted pulses from water, actual sheep-is-eating detection. JCE, 22-9-2018	
			sheepWaterPulse->setValue( (uint8_t) data[70], t-15); // Pulses are 30 seconds extended. Substracting 15 seconds of measurement time makes the pulses symmetric. JCE, 2-9-2018
			sheepFoodPresence->setValue(data[71] & (1 << 0), t); 
	
			// Added, jce, 22-6-2019		
			memcpy(&rain_ticks, data + 76, 4);
			rain_ticks = be32toh(rain_ticks);
			rain_bucket_tips->setValue(rain_ticks, t);		

			memcpy(&f, data + 80, 4);
			f = beftoh(f);
			rain_count->setValue(f, t);	
		
			// Added, JCE, 4-5-2022
			memcpy(&f, data + 84, 4);
			f = beftoh(f);
			room_CO->setValue(f, t);

			memcpy(&f, data + 88, 4);
			f = beftoh(f);
			room_smoke_temp->setValue(f, t);

			// Added, JCE, 7-9-2022
			memcpy(&f, data + 104, 4);
			f = beftoh(f);
			Bed_B1->setValue(f, t);

			memcpy(&f, data + 108, 4);
			f = beftoh(f);
			Bed_B2->setValue(f, t);

			memcpy(&f, data + 112, 4);
			f = beftoh(f);
			Bed_B3->setValue(f, t);

			memcpy(&f, data + 116, 4);
			f = beftoh(f);
			Bed_B4->setValue(f, t);

		}
	}

	//uint16_t header_scancounter = be16toh(&((uint16_t*)ptr));
	//uint16_t header_scancounter = be16toh(&((uint16_t*)ptr));
	//
	// Write part
	
	
	// Prepare the data area to be accepted by the PLC
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

	}

void interface_S1200::setOut(out* o, float v){
	o->setValue(v);
	}
