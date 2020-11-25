#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_sht3x.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"

#include <arpa/inet.h>
#include <endian.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <math.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

interface_sht3x::interface_sht3x(string d, string n, float i, string _dev_path, uint8_t _i2c_id):interface(d, n, i), dev_path(_dev_path), i2c_id(_i2c_id)
{
	temp = new in(getDescriptor() + "_temp", getName() + " temperature", "degC", 2);
	rh = new in(getDescriptor() + "_rh", getName() + " rel humidity", "%", 2);
	dewp = new in(getDescriptor() + "_dewp", getName() + " dewpoint", "degC", 1);
	ah = new in(getDescriptor() + "_ah", getName() + " abs humidity", "g/m3", 2);

	i2c_device = open(dev_path.c_str(), O_RDWR);
	if (i2c_device < 0)
		printf("Unable to open I2C bus %s\n", dev_path.c_str());
	ioctl(i2c_device, I2C_SLAVE, i2c_id);
}

interface_sht3x::~interface_sht3x()
{
	if (i2c_device >= 0)
		close(i2c_device);
	delete(ah);
	delete(dewp);
	delete(rh);
	delete(temp);
}

void interface_sht3x::getIns()
{
	uint8_t data[6];
	double t = now();

	// High repeatability measurement command
	data[0] = 0x2c;
	data[1] = 0x06;
	write(i2c_device, data, 2);

//	sleep(1);

	bool ok = read(i2c_device, data, 6) == 6;
	if (ok)
		ok = generate_crc(data, 2) == data[2] && generate_crc(data+3, 2) == data[5];
	if (ok)
	{
		float ft, frh, fah, fdp;
		uint16_t st, srh;
		float n, P, V, R, T, M, a, b, y;
		st =  256 * data[0] + data[1];
		srh = 256 * data[3] + data[4];
		ft = -45.0 + 175.0 * st / 65535;
		frh = 100.0 * srh / 65535;
		temp->setValue(ft, t);
		rh->setValue(frh, t);

		// Theoretical water vapour pressure at given temperature, according to Clausius-Clapeyron
		// and wikipedia: https://en.wikipedia.org/wiki/Vapour_pressure_of_water
		// P: Vapour pressure of water [kPa]
		// T: Temperature [degC]
		T = ft;
		P = 0.61094 * exp( (17.625 * T) / (T + 243.04) );
		
		// The actual vapour pressure is relative to this maximum.
		P = P * frh / 100;

		// And from kPa to Pa
		P = P * 1000;

		// Temperature translated to Kelvin
		T = T + 273.15;

		// With the ideal gas law we can now calculate to a concentration of particles.
		// PV = nRT, where:
		// P = vapour pressure [Pa]
		// V = volume [m3]
		// n = number of particles [mol]
		// R = gas constant, 8.3145 [J / mol / K]
		// T = temperature [K]
		V = 1.0;
		R = 8.3145;
		n = P * V / (R * T);

		// Calculate back to weight
		// M = molar weight of water, 18.01528 [g]
		M = 18.01528;
		fah = n * M;
		ah->setValue(fah, t);

		// Dewpoint from wikipedia as well.
		// nl.wikipedia.org/wiki/Dauwpunt
		// ft = temperature [degC]
		// frh = relative humidity [0-100%]
		// y = intermediate value
		// a, b = constantes from the formula
		a = 17.27;
		b = 237.7;
		y = (a * ft) / (b + ft) + log(frh/100);
		fdp = b * y / (a - y);
		dewp->setValue(fdp, t);
	}

	if (not ok)
	{
		temp->setValid(false);
		rh->setValid(false);
		dewp->setValid(false);
		ah->setValid(false);
	}
}

void interface_sht3x::setOut(out* /*o*/, float /*v*/)
{
}

// Obtained from sensirion reference implementation, V 5.2.0.
#define CRC8_INIT 0xFF
#define CRC8_POLYNOMIAL 0x31
uint8_t interface_sht3x::generate_crc(const uint8_t *data, uint16_t count)
{
   uint16_t current_byte;
    uint8_t crc = CRC8_INIT;
    uint8_t crc_bit;

    /* calculates 8-Bit checksum with given polynomial */
    for (current_byte = 0; current_byte < count; ++current_byte) {
        crc ^= (data[current_byte]);
        for (crc_bit = 8; crc_bit > 0; --crc_bit) {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}




