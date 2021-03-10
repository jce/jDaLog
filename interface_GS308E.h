#ifndef HAVE_GS308H_H
#define HAVE_GS308H_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "stringStore.h"

class interface_gs308e : public interface{
	public:
		interface_gs308e(const std::string, const std::string, const std::string); // descr, name, ip-as-string
		~interface_gs308e();
		void getIns();
		in *inp[24];	// rx 1-8, tx 1-8, er 1-8.
	private:
		double d_prev[3*8];	// previous rx 1-8, tx 1-8, er 1-8 readout.
		const std::string filestr;
		double t_prev;
	};

// Factory
void interface_gs308e_from_json(const char *, const char *, json_t *json); // id, name, json.

#endif // HAVE_GS308H_H
