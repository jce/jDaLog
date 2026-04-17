#ifndef HAVE_MB_H
#define HAVE_MB_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "stringStore.h"
#include "modbus/modbus.h"
#include <map>

typedef enum mb_regtype	
	{
		mb_none, 
		mb_coil, 
		mb_status, 
		mb_input, 
		mb_holding, 
		mb_regtype_num 
	} mb_regtype;

extern const char* mb_regtype_str[mb_regtype_num];

typedef enum mb_datatype 
	{
		mbd_none, 
		mbd_bool, 
		mbd_uint16, 
		mbd_int16, 
		mbd_uint32r, 
		mbd_int32r, 
		mbd_uint32, 
		mbd_int32, 
		mbd_uint64, 
		mbd_int64, 
		mbd_float16, 
		mbd_float32, 
		mbd_float64, 
		mbd_num
	} mb_datatype;

extern const char* mb_datatype_str[mbd_num];
extern const uint8_t mb_datatype_len[mbd_num];

typedef enum mb_comtype
	{ 
		mbc_none, 
		mbc_tcp, 
		mbc_rtu, 
		mbc_num 
	} mb_comtype;

typedef uint8_t mb_id;
typedef uint16_t mb_offset;

typedef struct mb_key
{
	mb_id id = 0;
	mb_regtype regtype = mb_none;
	mb_offset offset = 0;
}mb_key;
bool operator<(const mb_key& l, const mb_key& r);

typedef double (*mb_dt_read)(uint16_t*);			// Reads the modbus input data into a double
typedef void (*mb_dt_write)(double, uint16_t*);		// Writes the modbus out data based on the double.

typedef struct reg_context
{
	struct reg_context *next; 			// Next scheduled item
	mb_key key;							// Register address
	mb_dt_read readconv = NULL;			// Read dataconversion function
	in *i = NULL;						// In pointer
	mb_dt_write writeconv = NULL;		// Write dataconversion function
	out *o = NULL;						// Out pointer
	float interval = 0.0;				// Interval that the register should be requested
	double time = 0.0;					// Next scheduled request.
	double a = 1.0;						// Scaling of readout to real value: real_value = a * readout_value + b
	double b = 0.0;
	reg_context *adj_before = NULL;		// If there is an adjacent register before, this is the pointer to its context.
	reg_context *adj_after = NULL;		// If there is an adjacent register after, this is the pointer to its context.
	mb_datatype datatype = mbd_none;	// Datatype of the represented field (only in case of registers)
	uint8_t len = 1;					// Length of the datatype in modbusregisters (only in case of registers)
	~reg_context();
} reg_context;

class interface_mb : public interface{
	public:
		interface_mb(string, string, string, uint16_t); // descr, name, ip-as-string, port
		interface_mb(string, string, string, int, char, int, int); // descr, name, device, baud, parity, data_bit, stop_bit
		~interface_mb();
		void setOut(out*, float);
		void start();
		void run();
	private:
		const string _ipstr;	
		uint16_t port;	
		const string device;
		int baud, data_bit, stop_bit;
		char parity;
		// based on register:
		map<mb_key, reg_context> reg;
		void link_adj_reg_contexts();
		// Based on outpointer, for out callbacks
		map<out*, mb_key> outreg;
		void build_outreg();
		// Based on time of next fetch. Linked list based on the "next" member.
		reg_context *schedule = NULL;
		void init_schedule();
		void reschedule(reg_context*);
		static double next_multiple(double val, double interval);	// Get the next multiple;
		const mb_comtype comtype;
		modbus_t* new_context();

	friend void interface_mb_from_json(json_t*);
	};

void interface_mb_from_json(json_t*);

#endif // HAVE_MB_H
