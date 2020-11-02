#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "interface_macp.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "out.h"
#include <iostream>
#include <cstdio>
#include <memory>
#include <regex>
#include <filesystem>	// C++17, directory iterator


using namespace std;

void read_ins_from_dir(const char* dir, const char* prefix)
{
	// Data is stored in dir/descriptor/floatlog.bin
	// Try and reconstruct any descriptors 
	for( auto& p: filesystem::directory_iterator(dir))
		if (p.is_directory())
		{
			string descr = p.path().stem().string();
			string indir = string(dir) + "/" + descr;
			descr = string(prefix) + descr;
			//printf("new in(%s, %s, %s)\n", indir.c_str(), descr.c_str(), prefix);
			new in(24, indir.c_str(), descr, prefix);
		}
}

