#ifndef HAVE_INTERFACE_PI_GPIO_H
#define HAVE_INTERFACE_PI_GPIO_H

#include "stdio.h"
#include <string>
#include "in.h"
#include <sys/mman.h>
#include "jansson.h"

typedef enum gpio_mode
{
	mode_no_change,
	mode_input,
	mode_output,
}	gpio_mode;

typedef enum pull_state
{
	pull_off = 0,
	pull_down = 1,
	pull_up = 2
}	pull_state;

class interface_pi_gpio : public interface{
	public:
		interface_pi_gpio(const string, const string, float); // descr, name, ip-as-string
		~interface_pi_gpio();
		void getIns();
		void conf_gpio(int gpio, gpio_mode mode, pull_state pull, int def, const char *descr, const char *name);
		void setOut(out *o, float v); 
	private:
		int gpioInitialise();
		void gpioSetMode(unsigned gpio, unsigned mode);
		int gpioGetMode(unsigned gpio);
		void gpioSetPullUpDown(unsigned gpio, pull_state pud);
		int gpioRead(unsigned gpio);
		void gpioWrite(unsigned gpio, unsigned level);

		volatile uint32_t *gpioReg;
		map<int, in*> inputs;		// Contains all inputs and outputs.
		map<out*, int> outputs;		// Contains only outputs.
	};

// Partial factory, reads a json describing the gpio's to read/write.
// gpio{
//	"1":{
//		"pull":"up", "down", "off"
//		"mode":"input", "output", "no_change"
//		"default":0, 1
//		"descriptor":"blbl"
//		"name":"blabla"
void read_gpios_from_json(interface_pi_gpio*, json_t*);

#endif // HAVE_INTERFACE_HOST_H
