#ifndef HAVE_IN_EQUATION_H
#define HAVE_IN_EQUATION_H

#include "equation.h"
#include "in.h"

// This is an calculated in. Typically a top level object. It is constructed after interface ins.
// JCE, 6-3-2022


class in_equation: public in{
	public:
		//in(const string); // descr
		in_equation(const std::string, equation *_eq, const std::string name = "", const std::string units = "", const unsigned int decimals = 0); // descr, name, units, decimals in floating point representation.
		~in_equation();		

		in_equation(const in_equation& obj_temp) = delete;
		in_equation& operator=(const in_equation& temp_obj) = delete;

		const equation *eq;
	};

#endif // HAVE_IN_EQUATION_H
