#ifndef HAVE_LOGIC_H
#define HAVE_LOGIC_H

#include "stdio.h"
#include <string>
#include "stringStore.h"
#include "out.h"
#include "outhost.h"

using namespace std;

class logic: public outhost{
	public:
		logic(const string, const string name = ""); // descr, units, name
		virtual ~logic();
		virtual void setOut(out*, float);	// Intended for being called by out instances. JCE, 16-7-13
		const string getDescriptor();
		const string getName();		// JCE, 20-6-13
		const string getNote();		// JCE, 20-6-13
		virtual void run();			// JCE, 28-8-13
		virtual void start() {};	// Starts thread / loads scheduler if appropriate.
		//virtual string getPageHtml();	// Function where the logic can write its own html page response. JCE, 25-9-13
		void setNote(string);		// JCE, 25-9-13
		virtual string make_page(struct webserver_ctx*);	// Function that allows the logic class to draw its own page.
	private:
		const string _descr;
		stringStore *_name, *_note; // JCE, 20-6-13
	};

extern map<string, logic*> logics;

#endif // HAVE_LOGIC_H
