#ifndef HAVE_LOGIC_COMPARE_H
#define HAVE_LOGIC_COMPARE_H

#include "stdio.h"
#include <string>
#include "stringStore.h"
#include "out.h"
#include "outhost.h"
#include "mongoose.h"
#include "logic.h"

using namespace std;

class logic_compare: public logic{
	public:
		logic_compare(const string, const string, in*, in*); // descr, name, in-of-interest
		~logic_compare();
		void setOut(out*, float){}	// Intended for being called by out instances. JCE, 16-7-13
		void run(){}			// JCE, 28-8-13
		int make_page(struct mg_connection*);	// Function that allows the logic class to draw its own page.
		in *in_a, *in_b;
	};

#endif // HAVE_LOGIC_COMPARE_H


