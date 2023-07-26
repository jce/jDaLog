#ifndef HAVE_INTERFACE_S7_H
#define HAVE_INTERFACE_S7_H

#define byte byte_override
#include "snap7.h"
#undef byte

#include "in.h"
#include "interface.h"
#include "out.h"
#include "stringStore.h"

#include <map>
#include "stdio.h"
#include <string>

// Generic driver class to read ins from an S7 data block
// using the Snap7 library.

typedef enum S7_conntype
	{
		S7_PG,
		S7_OP,
		S7_basic,
		S7_connnum
	} S7_conntype;
extern const char* S7_conntype_str[S7_connnum];

typedef enum S7_regtype
	{
		S7_bool,
		S7_uint8,
		S7_int8,
		S7_uint16,
		S7_int16,
		S7_uint32,
		S7_int32,
		S7_uint64,
		S7_int64,
		S7_float32,
		S7_float64,
		S7_regnum
	} S7_regtype;
extern const char* S7_regtype_str[S7_regnum];
extern const int S7_regtype_len[S7_regnum];

typedef struct S7_key
{
	uint16_t db = 0;
	uint16_t byte = 0;
	uint16_t bit = 0;
} S7_key;
bool operator<(const S7_key& l, const S7_key& r);
		

typedef struct S7_io 
	{
		struct S7_io *next = NULL;			// Next scheduled item
		struct S7_key key;
		S7_regtype type;
		float read_interval;
		double next_scheduled_time = 0;
		double a, b;
		in* i = NULL;
		//bool has_validity_bit;
		//uint16_t validity_offset;
		//uint16_t validity_bit_offset;
		~S7_io();
	} S7_io;

class interface_S7 : public interface{
	public:
		interface_S7(const std::string, const std::string, float, const std::string, S7_conntype, uint16_t, uint16_t); // descr, name, ip-as-string, connection type, racknumber, slotnumber
		~interface_S7();
		void getIns();
		std::map<std::string, in*> ins;
		void start();
		void run();
	private:
		const std::string ipstr;
		const S7_conntype conntype;
		const uint16_t rack;
		const uint16_t slot;
 
        S7Object PLC;
		std::map<S7_key, S7_io> ios;
		S7_io *schedule = NULL;	// S7_io structures in chronological order
		double next_multiple(double, double);
		void init_schedule();
		void reschedule(S7_io*, double);	 
		in *latency;

	friend void interface_S7_from_json(const json_t*);
	};

void interface_S7_from_json(const json_t*);

#endif // HAVE_INTERFACE_S7_H
