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
	rh = new in(getDescriptor() + "_rh", getName() + " humidity", "%", 2);
	dewp = new in(getDescriptor() + "_dewp", getName() + " dewpoint", "degC", 1);
	ah = new in(getDescriptor() + "_ah", getName() + " humidity", "g/m3", 1);

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

	sleep(1);

	if (read(i2c_device, data, 6) != 6)
	{
		printf("jeu\n");



	}
	else	// Read failed
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




