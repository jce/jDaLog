#ifndef HAVE_WEBGUI_PAGE_H
#define HAVE_WEBGUI_PAGE_H

#include "in.h"
#include "out.h"
#include "webgui.h"

#include <map>
#include <string>
#include <vector>

typedef struct s_page
{
	std::string html;
	bool has_form = false;
	bool has_header = true;
	bool has_footer = true;
} t_page;

//extern std::map<std::string, t_page> page;

// Build the "page" struct from the configuration json. Returns 1 for
// success.
int build_page_data_from_json(json_t*);

// Check if a page exists with this URL.
bool page_exists(const string&);

// Check if a page has a header and/or footer
bool page_has_header(const string&);
bool page_has_footer(const string&);

// Build the page for given URL and 
// post data (implitit numeric id:content)
std::string build_page(const string&, vector<std::string>);

#endif // HAVE_WEBGUI_PAGE_H
