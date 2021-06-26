#ifndef HAVE_PIREG_H
#define HAVE_PIREG_H

#include "logic_modulator.h"

class logic_pi_reg: public logic 
{
	public:
		logic_pi_reg(const std::string, const std::string, in*, out*, const std::string, in* = NULL, in* = NULL, in* = NULL, in* = NULL, in* = NULL, in* = NULL); // bla, bla, measurement, actuator, "+" if actuator is incrementing the measurement, "-" if actuator is decrementing measurement, setpoint, P factor, I sum that actuates 1, Imin, Imax.
		~logic_pi_reg();
		void run();
		CC(logic_pi_reg, run);
		int make_page(struct mg_connection*);
	private:
		in *meas, *sp, *P, *I1, *Imin, *Imax, *Iratelim, *e, *esum;
		out *act;
		bool dir_is_increment;
		bool mysp = false, myP = false, myI1 = false, myImin = false, myImax = false, myIratelim;
		double tprev = 0;
		std::mutex runmutex;
	};

#endif // HAVE_PIREG_H
