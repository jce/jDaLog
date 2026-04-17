#ifndef HAVE_INTERFACE_FIJNSTOF_H
#define HAVE_INTERFACE_FIJNSTOF_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "out.h"
#include "stringStore.h"

class interface_fijnstof : public interface
{
	public:
		interface_fijnstof(const std::string, const std::string, float interval, const std::string); // descr, name, ip-as-string
		~interface_fijnstof();
		void getIns();
		in *pm2, *pm10, *temp, *rh, *pressure, *wifi_str, *wifi_qua, *samples, *time_sending, *fw_ver, *errors, *latency;
	private:
		const std::string _ipstr;
		int nr_samples = 0;
		double time_at_last_measurement = 0.0;	
};
interface_fijnstof* interface_fijnstof_from_json(const json_t*);
#endif // HAVE_INTERFACE_FIJNSTOF_H
