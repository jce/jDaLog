#ifndef HAVE_LOGIC_COMPARE_H
#define HAVE_LOGIC_COMPARE_H

#include "stdio.h"
#include <string>
#include "stringStore.h"
#include "out.h"
#include "outhost.h"
#include "logic.h"
#include <list>

class logic_compare: public logic{
	public:
		logic_compare(const std::string, const std::string, list<in*>); // descr, name, in-of-interest
		~logic_compare();
		void setOut(out*, float){}	// Intended for being called by out instances. JCE, 16-7-13
		void run(){}			// JCE, 28-8-13
		string make_page(struct webserver_ctx*);	// Function that allows the logic class to draw its own page.
		list<in*> inlist;		// List of ins, supplied in constructor.
		unsigned int gw = 0;	// Graph width. def_w is assumed if zero.
		unsigned int gh = 0;	// Graph heigth. def_h is assumed if zero.
	};

#endif // HAVE_LOGIC_COMPARE_H


