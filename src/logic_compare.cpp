#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "logic_compare.h"
#include "main.h"
#include "stringStore.h"
#include "out.h"
#include "webserver.h"
#include <list>
#include <ctime>
#include "float.h"

using namespace std;

logic_compare::logic_compare(const std::string d, const std::string n, list<in*>_il) : logic(d, n), inlist(_il)
{
}

logic_compare::~logic_compare()
{
}

string logic_compare::make_page(struct webserver_ctx *ctx)
{
	unsigned int w = gw;
	if (!w)
		w = ctx->srv->def_w;
	unsigned int h = gh;
	if (!h)
		h = ctx->srv->def_h;

	string rv = "comparing:<br>\n";
	for (auto i = inlist.begin(); i != inlist.end(); i++)
		rv += make_in_link(*i) + " ";
	rv += "<br><hr>\n";
	rv += make_image_line(inlist, now() - 60, now(), w, h);
	rv += make_image_line(inlist, now() - 3600, now(), w, h);
	rv += make_image_line(inlist, now() - 24*3600, now(), w, h);
	rv += make_image_line(inlist, now() - 7*24*3600, now(), w, h);
	rv += make_image_line(inlist, now() - 4*7*24*3600, now(), w, h);
	return rv;
}

