
// Addition to tcFarmControl, where a page can be described in the
// config.json file. Intention is to have custom pages to build simple
// user interfaces for custom deployed projects. The pages can use 
// custom tags to display in/out related information.

// The config part should look like this:
// webgui_page:
// {
//		"url":"html_content",
//		"url2":"html_content"
// }

#include "mytime.h"
#include "webgui.h"
#include "webgui_page.h"

#include <list>
#include <string>

#define DBG(...) { printf(__VA_ARGS__); printf("\n"); }
//#define DBG(...)

using namespace std;

map<string, t_page> page;

int build_page_data_from_json(json_t* json)
{
	const char *key;
	json_t *value;

	json_object_foreach(json, key, value) 
	{
		if (not json_is_string(value))
		{
			printf("Interpretation of page %s failed: Content not a string.\n", key);
			return 0;
		}
		page[string(key)] = {.html = string(json_string_value(value))};
		page[string(key)].has_form = page[string(key)].html.find("fc_input") != string::npos ;
		page[string(key)].has_header = page[string(key)].html.find("fc_noheader") == string::npos ;
		page[string(key)].has_footer = page[string(key)].html.find("fc_nofooter") == string::npos ;
	}
	return 1;
}

bool page_exists(const string &url)
{
	return page.count(url);
}

bool page_has_header(const string &url)
{
	if (not page_exists(url))
		return false;
	return page[url].has_header;
}

bool page_has_footer(const string &url)
{
	if (not page_exists(url))
		return false;
	return page[url].has_footer;
}

// Given a string
// < someword     hey = "tralala" or = 4 >
// translate to a tag (someword) and map:
// options[hey] = "tralala"
// options[or] = "4"

// Finds the begin of the first word based on common whitespace characters
size_t first_word_start( const string &line, size_t pos)
{
	return line.find_first_not_of(" \t\r\n", pos);
}

// Find the end of the current word, based on common whitespace characters
size_t first_word_end( const string &line, size_t pos)
{
	return line.find_first_of(" \t\r\n", pos);
}

// Returns the first word from a string
string first_word(const string &line, size_t pos = 0)
{
	size_t start = first_word_start(line, pos);
	size_t end = first_word_end(line, start);
	return string(line, start, end-start);
}

// Returns the first word after identifier =, or the words
// between quotes after identifier = 
string parameter_lookup(const string &line, const string tag)
{
	size_t pos = line.find(tag);
	if (pos == string::npos)
		return "";
	pos += tag.size();
		
	pos = line.find('=', pos);
	if (pos == string::npos)
		return "";
	pos +=1; 
	
	string rv = first_word(line, pos);
	
	// Detect if the word is quoted
	if (rv[0] == '"')
	{
		size_t start = line.find('"', pos);
		size_t end = line.find('"', start+1);
		rv = string(line, start+1, end-start-1);
	}	
		
	return rv;
}

string build_page(const string &url, vector<std::string> options)
{
	unsigned int post_item = 0;
	string html;
	
	if (page_exists(url))
	{
		if (page[url].has_form)
			html += "<form method=\"POST\">";
			
		html += page[url].html;
	
		// Now find the custom tags and replace them
		size_t begin = 0, end = 0, pos = 0;
		while (pos <= html.size())
		{
			begin = html.find("<", pos);
			end = html.find(">", begin);
			string tagstr = string(html, begin+1, end-begin-1);
			string firstword = first_word(tagstr);

			if (end > html.size())
				break;
			
			string replace = "";
			
			if (first_word(tagstr) == "fc_graph")
			{
				list<in*> ins;
				in *i;
				i = get_in(parameter_lookup(tagstr, "in")); if (i) ins.push_back(i);
				i = get_in(parameter_lookup(tagstr, "in1"));if (i) ins.push_back(i);
				i = get_in(parameter_lookup(tagstr, "in2"));if (i) ins.push_back(i);
				i = get_in(parameter_lookup(tagstr, "in3"));if (i) ins.push_back(i);
				i = get_in(parameter_lookup(tagstr, "in4"));if (i) ins.push_back(i);
				
				double tmin = now() - 3600;
				string par = parameter_lookup(tagstr, "time");

				if (par != "")
					try { tmin = now() - stod(par); } catch (...) {}
				
				par = parameter_lookup(tagstr, "tmin");
				if (par != "")
					try { tmin = stod(par);	} catch (...) {}

				double tmax = now();
				par = parameter_lookup(tagstr, "tmax");
				if (par != "")
					try { tmax = stod(par); } catch (...) {}
		
				unsigned int w = def_w;
				unsigned int h = def_h;

				par = parameter_lookup(tagstr, "width");
				if (par != "")
					try { w = stod(par); } catch (...) {}

				par = parameter_lookup(tagstr, "height");
				if (par != "")
					try { h = stod(par);	} catch (...) {}			

				string title = parameter_lookup(tagstr, "title");
				if (title == "" and ins.size() == 1)
					title = (*ins.begin())->getName();
					
				bool legend = parameter_lookup(tagstr, "legend") != "no";

				if (ins.size() >= 1)
					replace = make_image_line(plotLines(ins, tmin, tmax, w, h, title, legend));
				else
					replace = "in " + parameter_lookup(tagstr, "in") + " is not found\n";

				html.erase(begin, end-begin+1);
				html.insert(begin, replace);
				end = begin + replace.size();
			}
						
			if (first_word(tagstr) == "fc_value")
			{
				in *i = get_in(parameter_lookup(tagstr, "in"));
				if (i)
				{
					if (i->isValid())
					{
						char buf[1024];
						if (parameter_lookup(tagstr, "units") == "no")
							snprintf(buf, 1024, "%.*f\n", i->getDecimals(), i->getValue());
						else
							snprintf(buf, 1024, "%.*f %s\n", i->getDecimals(), i->getValue(), i->getUnits().c_str());
						replace = buf;
					}
					else
						replace = "in is not valid\n";
				}
				else
					replace = "in " + parameter_lookup(tagstr, "in") + " is not found\br";
					
				html.erase(begin, end-begin+1);
				html.insert(begin, replace);
				end = begin + replace.size();
			}
			
			if (first_word(tagstr) == "fc_input")
			{

				in *i = get_in(parameter_lookup(tagstr, "in"));
				if (i)
				{
					if (options.size() >= post_item + 1)
						i->setValue(stod(options[post_item]));
					char buf[1024];
						snprintf(buf, 1024, "%.*f", i->getDecimals(), i->getValue());	
						
					string units;
					if (parameter_lookup(tagstr, "units") != "no")
						units = i->getUnits() + " ";
						
					replace = "<INPUT type=\"TEXT\" name=\"" + to_string(post_item) + "\" value=\"" + buf + "\"> " + units + "\n";
				}
				else
					replace = "in " + parameter_lookup(tagstr, "in") + " is not found";

				post_item++;
				html.erase(begin, end-begin+1);	
				html.insert(begin, replace);
				end = begin + replace.size();
			}

			
			pos = end;
		}
		
		if (page[url].has_form)
			html += "<INPUT type=\"submit\" name=\"Submit\" value=\"Submit\"></form>";
	}
	
	return html;
}

