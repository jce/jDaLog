#ifndef HAVE_MB_H
#define HAVE_MB_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "stringStore.h"
#include "modbus/modbus.h"
#include <map>

typedef enum mb_regtype	{mb_none, mb_coil, mb_status, mb_input, mb_holding, mb_regtype_num } mb_regtype;
extern const char* mb_regtype_str[mb_regtype_num];
typedef uint8_t mb_id;
typedef uint16_t mb_offset;
typedef struct mb_key
{
	mb_id id = 0;
	mb_regtype regtype = mb_none;
	mb_offset offset = 0;
}mb_key;
bool operator<(const mb_key& l, const mb_key& r);

typedef struct reg_context
{
	struct reg_context *next; 	// Next scheduled item
	mb_key key;			// Register address
	in *i = NULL;			// In pointer
	out *o = NULL;			// Out pointer
	float interval = 0.0;		// Interval that the register should be requested
	double time = 0.0;		// Next scheduled request.
	reg_context *adj_before = NULL;	// If there is an adjacent register before, this is the pointer to its context.
	reg_context *adj_after = NULL;	// If there is an adjacent register after, this is the pointer to its context.
} reg_context;

class interface_mb : public interface{
	public:
		interface_mb(const string, const string, const string, uint16_t); // descr, name, ip-as-string, port
		~interface_mb();
		void getIns();
		void setOut(out*, float);
		void start();
		void run();
	private:
		const string _ipstr;
		uint16_t port = MODBUS_TCP_DEFAULT_PORT;	
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
		reg_context *schedule = NULL;
		void init_schedule();
		void reschedule(reg_context*);
		static double next_multiple(double val, double interval);	// Get the next multiple;

	friend void interface_mb_from_json(const char*, const char*, json_t*);
	};

void interface_mb_from_json(const char*, const char*, json_t*);

#endif // HAVE_MB_H
