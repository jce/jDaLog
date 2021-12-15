#ifndef HAVE_FRITZ_H
#define HAVE_FRITZ_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "stringStore.h"

class interface_fritz : public interface{
	public:
		interface_fritz(const std::string, const std::string); // descr, name
		~interface_fritz();
		void getIns();
	private:
		in *ul, *dl;
		unsigned long long txrate_prev = 0, rxrate_prev = 0;
	};

// Factory
void interface_fritz_from_json(const char *, const char *, json_t *json); // id, name, json.

#endif // HAVE_FRITZ_H
