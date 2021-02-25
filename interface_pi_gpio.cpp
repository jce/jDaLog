#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_pi_gpio.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "sys/statvfs.h" // statvfs()
#include "sys/resource.h" // getrusage
#include <sys/types.h> // getpid()
#include <unistd.h> // getpid() // sysconf
#include <stdint.h>
#include "unistd.h" // sysconf()
#include <iostream>
#include <cstdio>
#include <memory>
#include "jansson.h"
#include <fcntl.h>
#include <sys/mman.h>

using namespace std;

#define GPSET0 7
#define GPSET1 8

#define GPCLR0 10
#define GPCLR1 11

#define GPLEV0 13
#define GPLEV1 14

#define GPPUD     37
#define GPPUDCLK0 38
#define GPPUDCLK1 39

#define PI_BANK (gpio>>5)
#define PI_BIT  (1<<(gpio&0x1F))

/* gpio modes. */

#define PI_INPUT  0
#define PI_OUTPUT 1
#define PI_ALT0   4
#define PI_ALT1   5
#define PI_ALT2   6
#define PI_ALT3   7
#define PI_ALT4   3
#define PI_ALT5   2


interface_pi_gpio::interface_pi_gpio(const string d, const string n, float i):interface(d, n, i)
{
	gpioInitialise();
}

interface_pi_gpio::~interface_pi_gpio()
{
	while (!inputs.empty())
	{
		delete( inputs.begin()->second );
		inputs.erase(inputs.begin());
	} 
}

void interface_pi_gpio::getIns()
{
	double t = now();
	for(auto i = inputs.begin(); i != inputs.end(); i++)
		i->second->setValue(gpioRead(i->first), t);
}

void interface_pi_gpio::conf_gpio(int gpio, gpio_mode mode, pull_state pull, int def, const char *descr, const char *name)
{
	//printf("conf_gpio(%d, %d, %d, %d, %s %s)\n", gpio, mode, pull, def, descr, name);
	if (inputs.count(gpio))
		return;

	char d[128], n[128];
	
	if (mode == mode_input)
		gpioSetMode(gpio, PI_INPUT);
	if (mode == mode_output)
		gpioSetMode(gpio, PI_OUTPUT);

	gpioSetPullUpDown(gpio, pull);
	gpioWrite(gpio, def);

	if (descr)
		snprintf(d, 128, "%s_%s", getDescriptor().c_str(), descr);
	else
		snprintf(d, 128, "%s_%02d", getDescriptor().c_str(), gpio);

	if (name)
		snprintf(n, 128, "%s %s", getDescriptor().c_str(), name);
	else
		snprintf(n, 128, "%s gpio %02d", getDescriptor().c_str(), gpio);

	if (mode != mode_output)
	{
		in* i = new in(d, n, "", 0);
		inputs[gpio] = i;
	}
	else // if (mode == mode_output)
	{
		out* o = new out(d, n, "", 0, (void*) this, 0, 1, 1);
		outputs[o] = gpio;
		inputs[gpio] = o;
	}
}

void interface_pi_gpio::setOut(out *o, float v)
{
	if ( outputs.count(o) )
	{
		gpioWrite(outputs[o], v >= 0.5);
		o->setValue(gpioRead(outputs[o]));
	}		
} 

// Inspiration derived (copied) from http://abyz.me.uk/rpi/pigpio/examples.html#Misc_minimal_gpio
// JCE, 11-11-2020
int interface_pi_gpio::gpioInitialise(void)
{
   int fd;
   fd = open("/dev/gpiomem", O_RDWR | O_SYNC) ;
   if (fd < 0)
   {
      fprintf(stderr, "failed to open /dev/gpiomem\n");
      return -1;
   }
   gpioReg = (uint32_t *)mmap(NULL, 0xB4, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
   close(fd);
   if (gpioReg == MAP_FAILED)
   {
      fprintf(stderr, "Bad, mmap failed\n");
      return -1;
   }
   return 0;
}

void interface_pi_gpio::gpioSetMode(unsigned gpio, unsigned mode)
{
   int reg, shift;
   reg   =  gpio/10;
   shift = (gpio%10) * 3;
   gpioReg[reg] = (gpioReg[reg] & ~(7<<shift)) | (mode<<shift);
}

int interface_pi_gpio::gpioGetMode(unsigned gpio)
{
   int reg, shift;
   reg   =  gpio/10;
   shift = (gpio%10) * 3;
   return (*(gpioReg + reg) >> shift) & 7;
}

void interface_pi_gpio::gpioSetPullUpDown(unsigned gpio, pull_state pud)
{
   *(gpioReg + GPPUD) = pud;
   usleep(20);
   *(gpioReg + GPPUDCLK0 + PI_BANK) = PI_BIT;
   usleep(20);
   *(gpioReg + GPPUD) = 0;
   *(gpioReg + GPPUDCLK0 + PI_BANK) = 0;
}

int interface_pi_gpio::gpioRead(unsigned gpio)
{
   if ((*(gpioReg + GPLEV0 + PI_BANK) & PI_BIT) != 0) return 1;
   else                                         return 0;
}

void interface_pi_gpio::gpioWrite(unsigned gpio, unsigned level)
{
   if (level == 0) *(gpioReg + GPCLR0 + PI_BANK) = PI_BIT;
   else            *(gpioReg + GPSET0 + PI_BANK) = PI_BIT;
}

// Partial factory, reads a json describing the gpio's to read/write.
// gpio{
//	"1":{
//		"pull":"up", "down", "off"
//		"mode":"input", "output", "no_change"
//		"default":0, 1
//		"descriptor":"blbl"
//		"name":"blabla"
void read_gpios_from_json(interface_pi_gpio *interface, json_t *json)
{
	const char *descr, *name, *pull_s, *key, *mode_s;
	int def = 0;
	pull_state pull = pull_off;
	gpio_mode mode = mode_no_change;		
	json_t *value, *j_def;
	json_object_foreach(json, key, value)
	{
		int gpio;
		if (sscanf(key, "%d", &gpio) != 1)
			continue;

		descr = json_string_value(json_object_get(value, "id"));
		name = json_string_value(json_object_get(value, "name"));
		mode_s = json_string_value(json_object_get(value, "mode"));
		pull_s = json_string_value(json_object_get(value, "pull"));

		if (mode_s && strcmp(mode_s, "input") == 0)
			mode = mode_input;
		if (mode_s && strcmp(mode_s, "output") == 0)
			mode = mode_output;

		if (pull_s && strcmp(pull_s, "up") == 0)
			pull = pull_up;
		if (pull_s && strcmp(pull_s, "down") == 0)
			pull = pull_down;

		j_def = json_object_get(value, "default");
		if (json_is_number(j_def))
			def = json_number_value(j_def);
		if (json_is_true(j_def))
			def = 1;

		interface->conf_gpio(gpio, mode, pull, def, descr, name);
	}
}

