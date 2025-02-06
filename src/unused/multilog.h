#ifndef HAVE_MULTILOG_H
#define HAVE_MULTILOG_H

#include "stdio.h"
#include <list>
#include <map>
#include <string>
#include <vector>
#include "pthread.h"
using namespace std;

class multilog{
	public:

		// Constructor and destructor
		multilog(string);
		~multilog();

		// Functions to store values to the multilog.
		// Field 1 contains the key, a unique string.
		// Field 2 contains the exact record time. Note that only newer times are
		//	acceptable on subsequent calls.
		// Field 3 contains the value.
		//void write(const string&, const double&, const bool&);		// 1
		//void write(const string&, const double&, const uint8_t&);	// 2
		//void write(const string&, const double&, const int8_t&);	// 3
		//void write(const string&, const double&, const char&);		// 4
		//void write(const string&, const double&, const uint16_t&);	// 5
		//void write(const string&, const double&, const int16_t&);	// 6
		//void write(const string&, const double&, const uint32_t&);	// 7
		//void write(const string&, const double&, const int32_t&);	// 8
		//void write(const string&, const double&, const uint64_t&);	// 9
		//void write(const string&, const double&, const int64_t&);	// 10
		void write(const string&, const double&, const float&);	// 11
		//void write(const string&, const double&, const double&);	// 12
		//void write(const string&, const double&, const string&);	// 13
		//void write(const string&, const double&, const char* const, const uint32_t len&);	// 13
		
		// Comments
		void setComment(const string&, const double&, const string&);
		//void setComment(const string&, const char* const, const uint32_t len&);

		// Validity
		// A logged key is automatically valid if it logs a value. No need to manually set it to valid.
		void setInvalid(const string&);

		// Intended to be called at a regular interval from a separate thread.
		void writeToFile();

		// Retrieving will follow later.
	//private: // Just commented this during testing.
		const string _pathAndName;	

		// Types of records
		const uint8_t type_is_bool = 1;
		const uint8_t type_is_uint8 = 2;
		const uint8_t type_is_int8 = 3;
		const uint8_t type_is_char = 4;
		const uint8_t type_is_uint16 = 5;
		const uint8_t type_is_int16 = 6;
		const uint8_t type_is_uint32 = 7;
		const uint8_t type_is_int32 = 8;
		const uint8_t type_is_uint64 = 9;
		const uint8_t type_is_int64 = 10;
		const uint8_t type_is_float = 11;
		const uint8_t type_is_double = 12;
		const uint8_t type_is_string = 13;


		// Writing context
		struct singleField {
			string name;			// Unique descriptor
			uint8_t id;				// ID in message file
			uint8_t type;			// One of the type_is_<typename> constants
			bool has_value;			// True if this context time has a value entered.
			bool bool_value;		// Value... Crap multiple types
			uint8_t uint8_value;
			int8_t int8_value;
			char char_value;
			uint16_t uint16_value;
			int16_t int16_value;
			uint32_t uint32_value;
			int32_t int32_value;
			uint64_t uint64_value;
			int64_t int64_value;
			float float_value;
			double double_value;
			string string_value;
			string comment;};

		struct s_frame {
			double time;
			map<string, singleField> fields;
			}

		vector<char> data;
		
		pthread_mutex_t _fileMutex, _memMutex;
		};

#endif // HAVE_MULTILOG_H
