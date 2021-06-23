#include "floatLog.h"
#include "logic_pi_reg.h"
#include "main.h"
#include "mytime.h"	// now() jeu
#include "out.h"
#include "timefunc.h"
#include "webin.h"
#include "webgui.h"	// plotlines

#include "stdio.h"
#include <string>
#include "string.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "unistd.h" // usleep
#include "math.h"	// pow

using namespace std;

// bla, bla, measurement, actuator, "+" if actuator is incrementing the measurement, "-" if actuator is decrementing measurement, setpoint, P factor, I sum that actuates 1, Imin, Imax.
logic_pi_reg::logic_pi_reg(const string d, const string n, in *_meas, out *_act, const string direction, in *_sp, in *_P, in *_I1, in *_Imin, in *_Imax):
	logic(d, n), 
	meas(_meas), 
	sp(_sp),
	P(_P),
	I1(_I1),
	Imin(_Imin),
	Imax(_Imax),
	act(_act),
	dir_is_increment(direction == "+")
{
	if (not sp)
	{
		mysp = true;
		sp = new webin(getDescriptor() + "_sp", getName() + " Setpoint", meas->getUnits(), meas->getDecimals());
	}
	if (not P)
	{
		myP = true;
		P = new webin(getDescriptor() + "_p", getName() + " P factor", "", meas->getDecimals());
	}
	if (not I1)
	{
		myI1 = true;
		I1 = new webin(getDescriptor() + "_i1", getName() + " Isum for act = 1", meas->getUnits() + "s", meas->getDecimals());
	}
	if (not Imin)
	{
		myImin = true;
		Imin = new webin(getDescriptor() + "_imin", getName() + " I min", I1->getUnits(), I1->getDecimals());
	}
	if (not Imax)
	{
		myImax = true;
		Imax = new webin(getDescriptor() + "_imax", getName() + " I max", I1->getUnits(), I1->getDecimals());
	}

	e = new in(d + "_e", n + " error", meas->getUnits(), meas->getDecimals());
	esum = new in(d + "_esum", n + " error sum", meas->getUnits() + "s", meas->getDecimals());
	tprev = get_time_monotonic();
	meas->register_callback_on_update(cc_run, this);
}

logic_pi_reg::~logic_pi_reg()
{
	if(myP)		delete P;
	if(myI1)	delete I1;
	if(myImin)	delete Imin;
	if(myImax)	delete Imax;
	if(mysp)	delete sp;
	delete e;
	delete esum;
}

void logic_pi_reg::run()
{
	printf("a");
	runmutex.lock();
	double t = get_time_monotonic();
	printf("b");
	double dt = t - tprev;
	printf("c");
	tprev = t;

	double measval = meas->getValue();
	double spval = sp->getValue();
	double eval = measval - spval;
	double I1val = I1->getValue();
	printf("d");

	e->setValue( eval );
	double esumval = esum->getValue() + dt * eval;
	double esummin = Imin->getValue() * I1val;
	double esummax = Imax->getValue() * I1val;
	if (esumval < esummin)	esumval = esummin;
	if (esumval > esummax)	esumval = esummax;
	esum->setValue(esumval);

	printf("e");
	if (dir_is_increment)
	{
	printf("f");
		act->setOut(-eval * P->getValue() + -esumval / I1val);
	printf("g");
	}
	else
	{
	printf("h");
		act->setOut(eval * P->getValue() + esumval / I1val);
	printf("i");
	}
	runmutex.unlock();
	printf("j");
}

int logic_pi_reg::make_page(struct mg_connection *conn)
{
	mg_printf(conn, "pi regulator page<br>\n");
	mg_printf(conn, "Measurement: %.*f %s<br>\n", meas->getDecimals(), meas->getValue(), meas->getUnits().c_str());
	mg_printf(conn, "Setpoint: %.*f %s<br>\n", sp->getDecimals(), sp->getValue(), sp->getUnits().c_str());
	mg_printf(conn, "error: %.*f %s<br>\n", e->getDecimals(), e->getValue(), e->getUnits().c_str());
	mg_printf(conn, "error sum: %.*f %s<br>\n", esum->getDecimals(), esum->getValue(), esum->getUnits().c_str());
	mg_printf(conn, "Actuator: %.*f %s<br>\n", act->getDecimals(), act->getValue(), act->getUnits().c_str());

	mg_printf(conn, "P factor: %.*f %s<br>\n", P->getDecimals(), P->getValue(), P->getUnits().c_str());
	mg_printf(conn, "I sum for 1 actuation: %.*f %s<br>\n", I1->getDecimals(), I1->getValue(), I1->getUnits().c_str());
	mg_printf(conn, "Error sum minimum: %.*f %s<br>\n", Imin->getDecimals(), Imin->getValue(), Imin->getUnits().c_str());
	mg_printf(conn, "Error sum maximum: %.*f %s<br>\n", Imax->getDecimals(), Imax->getValue(), Imax->getUnits().c_str());

	string line;
 	line = make_image_line(plotLines(sp, meas, act, esum, now() - 3600, now(), 1280, 300, ""));
	mg_printf(conn, line.c_str());
 	line = make_image_line(plotLines(sp, meas, act, esum, now() - 24 * 3600, now(), 1280, 300, ""));
	mg_printf(conn, line.c_str());
 	line = make_image_line(plotLines(sp, meas, act, esum, now() - 7*24 * 3600, now(), 1280, 300, ""));
	mg_printf(conn, line.c_str());
	return 1;
}

