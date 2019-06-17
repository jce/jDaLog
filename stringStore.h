#ifndef HAVE_STRINGSTORE_H
#define HAVE_STRINGSTORE_H

#include "stdio.h"
#include <string>

using namespace std;

class stringStore{
	public:
		stringStore(string);
		~stringStore();
		void setString(string);
		//void setString(char*);
		string getString();
	private:
		void _storeToFile();
		void _loadFromFile();
		string _data;
		string _pathAndName;};

#endif // HAVE_STRINGTORE_H
