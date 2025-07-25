#ifndef HAVE_INTERFACE_DCMR_SENSOR_H
#define HAVE_INTERFACE_DCMR_SENSOR_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "out.h"
#include "stringStore.h"

class interface_dcmr_sensor : public interface
{
	public:
		interface_dcmr_sensor(const std::string, const std::string, float interval, const std::string); // descr, name, ip-as-string
		~interface_dcmr_sensor();
		void getIns();
		in *LAeq, *LAmin, *LAmax, *wifi_str, *wifi_qua, *latency;
	private:
		const std::string _ipstr;
		int nr_samples = 0;
		double time_at_last_measurement = 0.0;	
};

#endif // HAVE_INTERFACE_DCMR_SENSOR_H
