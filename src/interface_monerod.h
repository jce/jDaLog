#ifndef HAVE_INTERFACE_MONEROD_H
#define HAVE_INTERFACE_MONEROD_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "out.h"
#include "stringStore.h"
#include <jansson.h> // JCE, 6-9-2018, https://jansson.readthedocs.io/en/2.11/gettingstarted.html#compiling-and-installing-jansson


using namespace std;

class interface_monerod : public interface{
	public:
		interface_monerod(const string, const string); // descr, name
		~interface_monerod();
		void getIns();
		in *responseTime;
		in *precipIntensity, *precipProbability, *temperature, *apparentTemperature, *dewPoint, *humidity, *pressure, *windSpeed, *windGust, *windBearing, *cloudCover, *uvIndex, *visibility, *ozone;
	
		void setOut(out*, float);

	private:
		const string _key;
		const float _lat, _lon;
		void json_to_in(json_t*, in*, const char*, float, double);
		void setAllInsInvalid();	
	};

#endif // HAVE_INTERFACE_MONEROD_H
