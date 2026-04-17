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
		void run();
		in *LAeq, *LAmin, *LAmax, *wifi_str, *wifi_qua, *latency;
	private:
		const std::string _ipstr;
};

interface_dcmr_sensor* interface_dcmr_sensor_from_json(const json_t*);

#endif // HAVE_INTERFACE_DCMR_SENSOR_H
