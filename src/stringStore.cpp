#include "stdio.h"
#include <string>
#include "string.h"
#include <fstream>
#include <sstream>

#include "stringStore.h"
using namespace std;

stringStore::stringStore(string pan): _data(""), _pathAndName(pan) {
	//pathAndName = pan;
	_loadFromFile();}

stringStore::~stringStore() {
	//storeToFile(); 
	}

void stringStore::setString(string s) {
	if (_data != s){
		_data = s;
		_storeToFile();
		}
	}

string stringStore::getString(){
	return _data;}

void stringStore::_storeToFile() {
	FILE *fp;
        fp = fopen(_pathAndName.c_str(), "w");
        if (fp){
		const char* st = _data.c_str();
        	fwrite(st, strlen(st), 1, fp);
        	fclose(fp);}}

void stringStore::_loadFromFile(){
	char i[2];
	i[1] = (char)  NULL;
	FILE *fp;
	fp = fopen(_pathAndName.c_str(), "r");
	_data = "";
	if (fp){
		while (fread(i, 1, 1, fp)){
			_data += i;}
		fclose(fp);}	// JCE, 16-7-13
	return;}	

/*
#include <iostream>
int main(){
	// test
	//
	stringStore s("test.txt");
	cout << s.getString();
	s.setString("test een bla die bla");
	cout << s.getString();
	s.setString("nog een test");
	cout << s.getString();
	}
*/





















