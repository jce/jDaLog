#ifndef HAVE_LOGIC_KM_H
#define HAVE_LOGIC_KM_H

#include "stdio.h"
#include <string>
#include "stringStore.h"
#include "out.h"
#include "outhost.h"
#include "logic.h"

using namespace std;

class logic_km: public logic{
	public:
		logic_km(const string, const string, in*); // descr, name, in-of-interest
		~logic_km();
		void setOut(out*, float){}	// Intended for being called by out instances. JCE, 16-7-13
		void run(){}			// JCE, 28-8-13
		int make_page(struct mg_connection*);	// Function that allows the logic class to draw its own page.
		in *inp;
	};

#endif // HAVE_LOGIC_FIJNSTOF_H


