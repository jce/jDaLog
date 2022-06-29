#ifndef HAVE_WEBGUI_H
#define HAVE_WEBGUI_H

#include <list>
#include <string>
#include "in.h"
#include "out.h"

// helper functions for building a page outside of webgui.cpp

// encapsulates an image url into an html link
string make_image_line(string);

// make an htm l link. String 1 is the linked url, string 2 the visible tekst. Returns the html code for the link.
string make_link(string, string text = "");
string make_in_link(in*);
string make_in_link(in*, const std::string);
string make_webin_link(in*, const std::string);
string make_out_link(out*);

// in*, tmin, tmax, x-pixels, y-pixels
string plotLine(in*, double, double, unsigned int, unsigned int);

// list<in*>, tmin, tmax, x-pixels, y-pixels, title
string plotLines(list<in*>, double, double, unsigned int, unsigned int, string = "", bool = true);
// convenience functions, 2, 3 and 4 ins instead of list
string plotLines(in*, double, double, unsigned int, unsigned int, string);
string plotLines(in*, in*, double, double, unsigned int, unsigned int, string);
string plotLines(in*, in*, in*, double, double, unsigned int, unsigned int, string);
string plotLines(in*, in*, in*, in*, double, double, unsigned int, unsigned int, string);
string plotLines(in*, in*, in*, in*, in*, double, double, unsigned int, unsigned int, string);

// web server management
void webGuiStart(string port = "8090");
void webGuiStop();
void deleteOldFiles();	// JCE, 5-7-13

// Default image size(s)
extern uint16_t def_w, def_h;

// Configuration
void config_webgui(json_t*);

#endif // HAVE_WEBGUI_H
