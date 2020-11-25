#ifndef HAVE_INTERFACE_SHT3x_H
#define HAVE_INTERFACE_SHT3c_H

#include "stdio.h"
#include <string>
#include "in.h"
#include "float.h"
#include "stdint.h" 

class interface_sht3x : public interface{
	public:
		interface_sht3x(std::string, std::string, float, std::string, uint8_t); // descr, name, device_path, i2c_id
		~interface_sht3x();
		void getIns();
		void setOut(out*, float);
		in *temp, *rh, *dewp, *ah;
	private:
		const std::string dev_path;
		int i2c_device = 0;		// device descriptor for opened hardware device
		const uint8_t i2c_id;

		uint8_t generate_crc(const uint8_t *data, uint16_t count);
	};

#endif // HAVE_INTERFACE_SHT3x_H
