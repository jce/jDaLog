#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_circulac.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "out.h"

#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

#define CHANNELS \
CH(Xa) \
CH(Ya) \
CH(Xb) \
CH(Yb) \
CH(Xc) \
CH(Yc) 

//#define DBG(...)
#define DBG(...) { printf(__VA_ARGS__); printf("\n"); }

using namespace std;

interface_circulac::interface_circulac(const string d, const string n, float i, const string _devstr):interface(d, n, i), devstr(_devstr)
{
	DBG("Created interface_circulac with name %s and device %s", getName().c_str(), devstr.c_str());

	#define CH(_X_) ins[#_X_] = new in(getDescriptor() + "_" + #_X_, getName() + " " + #_X_, "m/s2", 4);
	CHANNELS
	#undef CH

}

interface_circulac::~interface_circulac()
{
}

#define MAXLEN 20
void interface_circulac::interpret(string &s)
{
	size_t r;
	int rv, v;
	if (s.length() >= MAXLEN)
		s.erase(0, s.length() - MAXLEN);

#define CH(_X_) \
{ \
	r = s.find(#_X_); \
	if (r != string::npos and (r + 3 + 4) <= s.length())\
	{	\
		rv =sscanf(s.c_str() + r + 3, "%d", &v); \
		if (rv > 0) \
		{ \
			ins[#_X_]->setValue(v, now()); \
			/*DBG("%s %d", #_X_, v);*/ \
		} \
		s.erase(0, r+4); \
	} \
}

	CHANNELS

#undef CH
}

void interface_circulac::start()
{
	run_flg = true;
	interface::start();
}

#define RECONNECTTIME	4
#define TIMEOUT			1

void interface_circulac::run()
{
	int rv;
	char c[2];
	c[1] = 0;
	string received;
	double lastrecv = now();

	while(run_flg)
	{
		bool backlog = true;
    	int fd = open(devstr.c_str(), O_RDONLY | O_NOCTTY | O_NDELAY);
    	// O_RDWR read/write, O_RDONLY read only
    	// O_NOCTTY we do not want to be the "controlling terminal"
    	// O_NDELAY we do not care for the DCD (data carrier detect) signal line
		if (fd <= 0)
		{
			DBG("opening serial port %s gives error %d: %s", devstr.c_str(), errno, strerror(errno));
			sleep(RECONNECTTIME);
			continue;
		}
		else
		{
			lastrecv = now();
			DBG("opening serial port %s opened.", devstr.c_str());
		}
    	struct termios fd_options;
    	tcgetattr(fd, &fd_options);

    	// 115200 baud
    	cfsetispeed(&fd_options, B115200);
    	cfsetospeed(&fd_options, B115200);

    	fd_options.c_cflag |= (CLOCAL | CREAD); // Enables receiver and do not become owner.

    	// 8N1
    	fd_options.c_cflag &= ~PARENB; // Not enable parity bit
    	fd_options.c_cflag &= ~CSTOPB; // Not enable two stop bits (leaves one stop bit)
    	fd_options.c_cflag &= ~CSIZE; // Clear all bits for data size
    	fd_options.c_cflag |= CS8; // Set data size to 8 bits

    	fd_options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    	// not ICANON no canonical input (so immediately give characters instead of waiting for newline)
    	// not ECHO does not echo input characters
    	// not ECHOE does not echo erase characters as BS-SP-BS
    	// not ISIG does not enable SIGINTR, SIGSUSP, SIGDSUSP and SIGQUIT signals

    	// Disable software flow control
    	fd_options.c_iflag &= ~(IXON | IXOFF | IXANY);

    	// Disable output processing
    	fd_options.c_oflag &= ~OPOST;

    	tcsetattr(fd, TCSANOW, &fd_options); // TCSANOW makes changes now without waiting for data to complete.
		
		while(run_flg)
		{
			rv = read(fd, &c, 1);
			if (rv > 0 and not backlog)
			{
				lastrecv = now();
				if (c[0] == '\r' or c[0] == '\n' or c[0] == '\t')
					c[0] = ' ';
				received.append(c);
				if (received.length() > 5)
					interpret(received);
			}
			if (rv <= 0)
			{
				backlog = false;
				usleep(100);
				if (now() > lastrecv + TIMEOUT)
				{
					DBG("Too long has passed since receiving a character. Maybe the USB was disconnected.");
					break;
				}
			}
		}

		DBG("Closing serial port %s", devstr.c_str());
		close(fd);
		if (run_flg)
			sleep(RECONNECTTIME);
	}
}


bool interface_circulac_from_json(const json_t *json)
{
	#define JSTR(_X_)	json_string_value(json_object_get(json, #_X_))
	const char *id = JSTR(id);
	const char *name = JSTR(name);
	if (not name)
		name = id;
	const char *devstr = JSTR(device);
	#undef JSTR

	if (id and devstr)
	{
		new interface_circulac(id, name, 1, devstr);
		return true;
	}
	return false;
	
}
