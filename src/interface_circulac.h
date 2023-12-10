#ifndef HAVE_INTERFACE_CIRCULAC_H
#define HAVE_INTERFACE_CIRCULAC_H

#include "stdio.h"
#include <string>

class interface_circulac : public interface{
	public:
		interface_circulac(const std::string, const std::string, float, const std::string); // descr, name, ip-as-string
		~interface_circulac();
		void start();
		void run();
	private:
		const std::string devstr;
		void interpret(std::string &);
	};


bool interface_circulac_from_json(const json_t*);

#endif // HAVE_INTERFACE_FRIDGE_H
