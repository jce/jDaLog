#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "logic_or.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "out.h"
#include "unistd.h" // usleep
#include "math.h"	// pow
#include "webin.h"
#include "webgui.h"	// plotlines

using namespace std;

//#define debug

logic_or::logic_or(const string d, const string n, out *o) : logic(d, n), _myOut(o){
	A = new out(getDescriptor() + "_A", getName() + " A", "", 0, this, 0, 1, 1);
	B = new out(getDescriptor() + "_B", getName() + " B", "", 0, this, 0, 1, 1);
	}

logic_or::~logic_or(){
	delete B;
	delete A;}

void logic_or::setOut(out* o, float v){
	#ifdef debug
	printf("logic_or's setOut() is called...\n");
	#endif
	if (o == A or o == B){
		o->setValue(v);
		#ifdef debug 
		printf("logic_or's setOut(): myOut before setting=%f\n", _myOut->getValue());
		#endif
		_myOut->setOut(A->getValue() == 1 or B->getValue() == 1);
		#ifdef debug 
		printf("logic_or's setOut(): A known out is supplied... A=%f, B=%f, myOut=%f\n", A->getValue(), B->getValue(), _myOut->getValue());
		#endif
		};
	}

int logic_or::make_page(struct mg_connection *conn){
	//mg_printf(conn, "OR logic for out's. Displaying the source out A, B and destination out<br>\n");
	//string line; line = make_image_line(plotLines(A, B, _myOut, now() - 3600, now(), 1280, 300, "A or B is result"));
	//mg_printf(conn, line.c_str());
	return 1;}

