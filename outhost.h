#ifndef HAVE_OUTHOST_H
#define HAVE_OUTHOST_H

#include "out.h"

// base class to inherit from, if the class has to have a setOut(out*, float)

class outhost{
	public:
		outhost(){}
		virtual ~outhost(){}
		virtual void setOut(out*, float){}	// Intended for being called by out instances. JCE, 16-7-13
	};


#endif // HAVE_OUTHOST_H
