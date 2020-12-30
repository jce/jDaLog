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
typedef struct mb_key
{
	mb_id id = 0;
	mb_regtype regtype = mb_none;
	mb_offset offset = 0;
}mb_key;
bool operator<(const mb_key& l, const mb_key& r);

struct reg_context;
typedef struct schedule_item
{
	struct schedule_item *next = NULL;
	double time = 0.0;
	mb_key key;
	struct reg_context *context = NULL;
} schedule_item;

typedef struct reg_context
{
	in *i = NULL;			// In pointer
	out *o = NULL;			// Out pointer
	float interval = 0.0;		// Interval that the register should be requested
	double time = 0.0;		// Next scheduled request.
	schedule_item si; 		// Allocated for use in the schedule.
	reg_context *adj_before = NULL;	// If there is an adjacent register before, this is the pointer to its context.
	reg_context *adj_after = NULL;	// If there is an adjacent register after, this is the pointer to its context.
} reg_context;

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
		map<mb_key, reg_context> reg;
		void link_adj_reg_contexts();
		// Based on outpointer, for out callbacks
		map<out*, mb_key> outreg;
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
