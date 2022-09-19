#ifndef HAVE_LOGIC_SLEEP_H
#define HAVE_LOGIC_SLEEP_H

#include "stdio.h"
#include <string>
#include "stringStore.h"
#include "out.h"
#include "outhost.h"
#include "mongoose.h"
#include "logic.h"

#include <list>

class logic_sleep: public logic{
	public:
		logic_sleep(const std::string, const std::string, std::list<in*>); // descr, name, in-of-interest
		~logic_sleep();
		void setOut(out*, float){}	// Intended for being called by out instances. JCE, 16-7-13
		void run(){}			// JCE, 28-8-13
		int make_page(struct mg_connection*);	// Function that allows the logic class to draw its own page.
		std::list<in*> inp;
	};

#endif // HAVE_LOGIC_FIJNSTOF_H


