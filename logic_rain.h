#ifndef HAVE_LOGIC_RAIN_H
#define HAVE_LOGIC_RAIN_H

#include "stdio.h"
#include <string>
#include "stringStore.h"
#include "out.h"
#include "outhost.h"
#include "mongoose.h"
#include "logic.h"

using namespace std;

class logic_rain: public logic{
	public:
		logic_rain(const string, const string); // descr, name
		~logic_rain();
		void setOut(out*, float){}	// Intended for being called by out instances. JCE, 16-7-13
		void run(){}			// JCE, 28-8-13
		int make_page(struct mg_connection*);	// Function that allows the logic class to draw its own page.
	};

#endif // HAVE_LOGIC_FIJNSTOF_H


