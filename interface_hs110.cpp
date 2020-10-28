#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_hs110.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"

#include <arpa/inet.h>
#include <endian.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

interface_hs110::interface_hs110(const string d, const string n, float i, const string ipstr):interface(d, n, i), _ipstr(ipstr)
{
	voltage = new in(getDescriptor() + "_u", getName() + " voltage", "V", 3);
	current = new in(getDescriptor() + "_i", getName() + " current", "A", 3);
	power = new in(getDescriptor() + "_p", getName() + " power", "W", 3);
	total_kwh = new in(getDescriptor() + "_tot", getName() + " power total", "kWh", 3);
	err_code = new in(getDescriptor() + "_err", getName() + " err_code");
	latency = new in(getDescriptor() + "_lt", getName() + " latency", "ms", 3);
	va = new in(getDescriptor() + "_va", getName() + " va", "VA", 3);
	pf = new in(getDescriptor() + "_pf", getName() + " pf", "", 5);
	whreadout = new in(getDescriptor() + "_whr", getName() + " wh readout", "wh");
	kWh_stored_at_startup = total_kwh->getValue();

	memset(&sa, 0, sizeof(sockaddr_in));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(_ipstr.c_str());
	sa.sin_port = htons(9999);
	start();
}

interface_hs110::~interface_hs110()
{
	delete(whreadout);
	delete(pf);
	delete(va);
	delete(voltage);
	delete(current);
	delete(power);
	delete(total_kwh);
	delete(err_code);
	delete(latency);
}

// Many thanks to these guys!
// https://www.softscheck.com/en/reverse-engineering-tp-link-hs110/
// https://github.com/softScheck/tplink-smartplug
// JCE, 29-9-2020

#define KEY 171

static void encrypt(char *str, size_t len)
{
	if (len >= 1)
		str[0] ^= KEY;
	if (len > 1)
		for (size_t i = 1; i < len; i++)
			str[i] ^= str[i-1];
}

static void decrypt(char *str, size_t len)
{
	if (len > 1)
		for (size_t i = len-1; i >= 1; i--)
			str[i] = str[i] ^ str[i-1];
	if (len >= 1)
		str[0] = str[0] ^ KEY;
}

/*
static void prp(double t, const char* s)
{
	//print debug point.
	// If the time is longer than treshold, print time and string.
	float treshold = 1.000;
	if ((now() - t) >= treshold)
	{
		printf("%.3f +%.3f %s\n", now(), now() - t, s);
	}
}*/
//prp(t, "a");
#define prp(_X_,_Y)

void interface_hs110::getIns()
{
//double q = now();
prp(q, "a");
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	fd_set set;
	int rv;
	ssize_t ans_len = 0;

prp(q, "b");
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (!sock)
		return;

	FD_ZERO(&set);
	FD_SET(sock, &set);
prp(q, "c");

	int opt = fcntl (sock, F_GETFL, NULL);
	fcntl(sock, F_SETFL, opt | O_NONBLOCK);	// Set to non blocking
	rv = connect(sock, (sockaddr*) &sa, sizeof(sockaddr_in));
	if (rv < 0 and errno == EINPROGRESS)
	{
		rv = select(sock+1, NULL, &set, NULL, &timeout);
		if (rv == 1)
			rv = 0;
	}
	
	if ( rv != 0 )
	{
		close(sock);
		return;
	}

	//fcntl(sock, F_SETFL, opt);	// Reset socket flags

prp(q, "d");
	// bla bla interactions
	double start = now();

prp(q, "e");
	const char *request = "{\"emeter\":{\"get_realtime\":{}}}";
	const size_t req_len = strlen(request);
	const uint32_t req_len_be = htobe32(req_len);

prp(q, "f");
	strcpy(buf, request);
	//printf("%s\n", buf);
	encrypt(buf, req_len);
	//printf("%s\n", buf);
prp(q, "g");
	write(sock, &req_len_be, 4);
prp(q, "h");
	write(sock, buf, req_len);
prp(q, "i");

prp(q, "j");
prp(q, "k");
	rv = select(sock+1, &set, NULL, NULL, &timeout);

prp(q, "l");
	if (rv > 0)
		ans_len = read(sock, buf, HS110_BUFSIZE);

prp(q, "m");
	close(sock);
prp(q, "n");
	double end = now();
	double t = (start + end) / 2;

prp(q, "o");
	//printf("%d\n", ans_len);
	if (ans_len >= 4)
	{	
		//printf("%.*s\n", ans_len, buf);
		decrypt(buf+4, ans_len - 4);
		buf[ans_len] = 0x00;
		//printf("%s\n", buf+4);
		
		float v, a, p, wh, err, tmpva, tmppf;
		//findFloatAfter(buf, "voltage_mv\":", voltage, t);		
		if (findFloatAfter(buf+4, "voltage_mv\":", v))
			voltage->setValue(v/1000, t);		
		else
			voltage->setValid(false);

		if (findFloatAfter(buf+4, "current_ma\":", a))
			current->setValue(a/1000, t);		
		else
			current->setValid(false);

		if (findFloatAfter(buf+4, "power_mw\":", p))
			power->setValue(p/1000, t);		
		else
			power->setValid(false);

		//if (findFloatAfter(buf+4, "total_wh\":", wh))
		//	total_kwh->setValue(wh/1000, t);		
		//else
		//	total_kwh->setValid(false);
		// It seems the meter does not store watthours over a restart. JCE, 3-10-2020
		if (findFloatAfter(buf+4, "total_wh\":", wh))
		{
			if (wh < Wh_hs110_last_readout)
			{
				kWh_stored_at_startup +=( Wh_hs110_last_readout - Wh_hs110_at_startup) / 1000;
				Wh_hs110_at_startup = wh;
			}
			Wh_hs110_last_readout = wh;

			total_kwh->setValue(kWh_stored_at_startup + (wh - Wh_hs110_at_startup)/1000, t);		
			whreadout->setValue(wh, t);
		}
		else
			total_kwh->setValid(false);

		if (findFloatAfter(buf+4, "err_code\":", err))
			err_code->setValue(err, t);		
		else
			err_code->setValid(false);

		if (voltage->isValid() and current->isValid())
		{
			tmpva = v * a / 1000000;
			va->setValue(tmpva, t);
			if (power->isValid())
			{
				if (tmpva != 0)
				{
					tmppf = p / 1000 / tmpva;
					if (tmppf > 1.5) 
						tmppf = 1.5;
					if (tmppf < 0) 
						tmppf = 0;
				}
				else
					tmppf = 0;
				pf->setValue(tmppf, t);
			}
			else
				pf->setValid(false);
		}
		else
		{
			va->setValid(false);
			pf->setValid(false);
		}
	}
	float l = (end - start) * 1000;
	if (l > 1000)
		l = 1000;
	latency->setValue(l, t); 
prp(q, "p");
}

