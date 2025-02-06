#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "logic.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "out.h"

using namespace std;

map<string, logic*> logics;

logic::logic(const string d, const string n) : _descr(d) 
{
	string path = tcDataDir;	
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	path += "/logic/";
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	path += d;
	mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
	_name = new stringStore(path + "/name.txt");
	//if (_name->getString() != n and n != "") _name->setString(n); // Commented. JCE, 11-7-13, build 481
	_name->setString(n);
	_note = new stringStore(path + "/note.txt");
	logics[_descr] = this ;
}

logic::~logic()
{
	#ifdef debug
		printf("Deleting logic %s...\n", _descr.c_str());
	#endif
	logics.erase(_descr);
	_name->~stringStore();
	_note->~stringStore();
	#ifdef debug
		printf("Logic %s is deleted\n", _descr.c_str());
	#endif
}

// Function intended for an out to call, if it needs to be set. The out itself cannot set the physical device, the logic has to do this.
void logic::setOut(out*, float){} // JCE, 16-7-13

const string logic::getDescriptor(){
	return _descr;}

const string logic::getName(){
	return _name->getString();}

const string logic::getNote(){
	return _note->getString();}

void logic::run(){}	// JCE, 28-8-13

//string logic::getPageHtml()	// Function where the logic can write its own html page response. JCE, 25-9-13
//	{return "<br>Sorry, this logic instance does not implement a page.<br>";}

void logic::setNote(string newNote){
	_note->setString(newNote);}

int logic::make_page(struct mg_connection *conn){
	mg_printf(conn, "<br>Sorry, this logic instance does not implement a page.<br>");
	return 1;
	}


