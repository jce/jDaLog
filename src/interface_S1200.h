#ifndef HAVE_INTERFACE_S1200_H
#define HAVE_INTERFACE_S1200_H


#define byte byte_override
#include "snap7.h"
#undef byte

#include "in.h"
#include "interface.h"
#include "out.h"
#include "stringStore.h"

#include "stdio.h"
#include <string>

//using namespace std;

// Base class for interfaces. An interface is responsible for acquiring the values/datas for in's. In the future also for setting out's. This is the base class, every in should be an expansion of this. Dont know if this one should be instantiated as well, but should be possible for testing purposes. JCE, 20-6-13

//typedef unsigned int S7Object;

class interface_S1200 : public interface{
	public:
		interface_S1200(const std::string, const std::string, float, const std::string); // descr, name, ip-as-string
		~interface_S1200();
		void getIns();
		in *latency;

		in *hyfstarts, *hyfruntime, *hyfpressure, *hyflont, *hyflofft;
		in *roomIOOnline, *roomMotionTop, *roomMotionDesk, *roomDoorOpen, *roomMotionCorridor, *roomPresence, *roomLight;
		in *chickenDeskMotion;
		in *chickenGatePosEst;
		in *scantime;
		in *roomTemp, *roomRH;
		in *rainDetector;
		in *roomComp, *roomSleepSw, *roomWindowFB, *roomLearnedPresence, *roomLearnedSleep;
		in *fishTankPump; // JCE, 30-8-2018		
		in *sheepWater, *sheepFoodTime; // JCE, 5-9-2018
		in *sheepWaterPulse, *sheepFoodPresence; // Abstracted pulses from water, actual sheep-is-eating detection. JCE, 22-9-2018	
		in *uptime_in; // JCE, 30-4-2019	
		in *rain_bucket_tips, *rain_count; // JCE, 22-6-2019
		in *room_CO, *room_smoke_temp;	// JCE, 4-5-2022

		//out *Q0_0;
		// Data.h
		// measurements
		//in *F_none, *F_ired, *F_red, *F_green, *F_blue, *temp_mC, *env_light, *pH_readout;

		// configurations, should be webin? for the moment the device is the master of these configurations
		//
		//in *I0_ired, *I0_red, *I0_green, *I0_blue, *D_led_sensor, *t_cycle, *n_cycles, *C_ired, *C_red, *C_green, *C_blue;
		//in *pH_a_pH, *pH_a_readout, *pH_b_pH, *pH_b_readout;

		// Calculation results
		//in *I_ired, *I_red, *I_green, *I_blue;
		//in *T_ired, *T_red, *T_green, *T_blue;
		//in *AC_ired, *AC_red, *AC_green, *AC_blue;  

		// Locally calculated results
		//in *BdivR;
		
		//in *CO2;
		//out *DC, *R0, *R1, *R2;
		//in *temp, *rh, *dewp, *SOt, *SOrh;
		void setOut(out*, float);
	private:
		const std::string _ipstr;
        S7Object PLC;
		uint32_t scancounter, writecounter;
		double lastReadTime;
		float lastScanTime;
	};

#endif // HAVE_INTERFACE_S1200_H
