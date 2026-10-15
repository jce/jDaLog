#ifndef HAVE_INTERFACE_CT003_H
#define HAVE_INTERFACE_CT003_H

#include "config.h"

#include "stdio.h"
#include <string>
#include "in.h"
#include "stringStore.h"


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


class interface_ct003 : public interface{
	public:
		interface_ct003(const string, const string, float, const char*); // descr, name, scan, ip-as-string
		~interface_ct003();
		void getIns();
		in *Pa, *Pb, *Pc, *Pd, *rssi, *info_idx, *low_in, *norm_in, *low_out, *norm_out, *dt;
	private:
        string ip;
        void ct003_parse_body(const string &body);
	};
interface_ct003* interface_ct003_from_json(const json_t*);

#endif // HAVE_INTERFACE_CT003_H
