#ifndef HAVE_INTERFACE_DCMR_H
#define HAVE_INTERFACE_DCMR_H

#include "config.h"

#include "stdio.h"
#include <string>
#include "in.h"
#include "stringStore.h"

class interface_dcmr : public interface{
	public:
		interface_dcmr(const string, const string, float); // descr, name, ip-as-string
		~interface_dcmr();
		void run();
		in *boslaan, *achterdijk, *veldkersweg, *pyramide, *vosmaerstraat, *havikenhof, *canniuslaan, *rubenslaan, *uiverstraat, *develden, *kasteelweg, *nachtegaallaan, *kruislaan, *kareldegrotelaan;
	private:
		double _prevT, head;
	};
interface_dcmr* interface_dcmr_from_json(const json_t*);

#endif // HAVE_INTERFACE_HOST_H
