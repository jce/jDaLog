#ifndef HAVE_MB_H
#define HAVE_MB_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "stringStore.h"
#include "modbus/modbus.h"
#include <map>

typedef enum mb_regtype	{mb_none, mb_coil, mb_status, mb_input, mb_holding } mb_regtype;
typedef uint8_t mb_id;
typedef uint16_t mb_offset;
typedef struct schedule_item
{
	struct schedule_item *next = NULL;
	double time = 0.0;
	mb_id id;
	mb_regtype regtype;
	mb_offset offset;
} schedule_item;

typedef struct reg_context
{
	in *i = NULL;			// In pointer
	out *o = NULL;			// Out pointer
	float interval = 0.0;	// Interval that the register should be requested
	double time = 0.0;		// Next scheduled request.
	schedule_item si; 		// Allocated for use in the schedule.
} reg_context;

typedef struct out_context
{
	mb_id id = 0;
	mb_regtype regtype = mb_coil;
	mb_offset offset = 0;
} out_context;

class interface_mb : public interface{
	public:
		interface_mb(const string, const string, const string); // descr, name, ip-as-string
		~interface_mb();
		void getIns();
		void makeModbusContext();
		void setOut(out*, float);
		void start();
		void run();
	private:
		const string _ipstr;
		modbus_t *_ctx;
		// Data is constructed ordered in different trees, depending on the way of use. The root tree would be
		// based on register:
		map<mb_id, map<mb_regtype, map<mb_offset, reg_context>>> reg;
		// Based on outpointer, for out callbacks
		map<out*, out_context> outreg;
		void build_outreg();
		// Based on time of next fetch
		//map<double, map<mb_id, map<mb_regtype, map<mb_offset,time_context>>>> timereg;
		//void build_timereg();
		// Lets do that different. The first register to fetch comes from one queue. Then it follows the "reg" struct to find additional
		// registers that can be fetched in the same request.
		schedule_item *schedule = NULL;
		void init_schedule();
		void reschedule(schedule_item*);
		static double next_multiple(double, double);	// Get the next multiple;

	friend void interface_mb_from_json(const char*, const char*, json_t*);
	};

void interface_mb_from_json(const char*, const char*, json_t*);

#endif // HAVE_MB_H
