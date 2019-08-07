# maybe read https://latedev.wordpress.com/2014/11/08/generic-makefiles-with-gcc-and-gnu-make/
# or https://bitbucket.org/neilb/genmake/src/ba351ff41d3cd0e1168c804965bfd733d6fa97b5/srcinc/makefile?at=default&fileviewer=file-view-default
# JCE, 16-10-2018
# Some more inspiration might be obtained from:
# https://stackoverflow.com/questions/231229/how-to-generate-a-makefile-with-source-in-sub-directories-using-just-one-makefil
# JCE, 29-10-2018
CC=gcc
CPP=g++
#CFLAGS=-I. -ldl -O2 -W -Wall -pedantic -pthread -L snap7-full-1.4.0/release/Linux/i386/glibc_2.21 -l snap7 -l curl -l modbus #-m32
CCFLAGS=-I. -W -Wall -std=c99 -g -O0 -Wno-psabi#-O3
CPPFLAGS=-I. -W -Wall -std=c++17 -g -O0 -Wno-psabi# -O3#-m32
#LFLAGS=-ldl -pthread -lcurl -lmodbus -lsnap7 -ljansson -lmicrohttpd -L/usr/lib/arm-linux-gnueabihf  -lmariadbclient 
LFLAGS=-ldl -pthread -lcurl -lmodbus -lsnap7 -ljansson -L/usr/lib/arm-linux-gnueabihf  -lmariadbclient 
OBJDIR=obj
INCDIRS=


CPPFILES := $(wildcard *.cpp)
CFILES := $(wildcard *.c)

OBJFILES_CPP := $(patsubst %.cpp, $(OBJDIR)/%.o, $(CPPFILES)) 
OBJFILES_C := $(patsubst %.c, $(OBJDIR)/%.o, $(CFILES))
OBJFILES := $(OBJFILES_CPP) $(OBJFILES_C)

DEPFILES_CPP := $(patsubst %.cpp, $(OBJDIR)/%.d, $(CPPFILES)) 
DEPFILES_C :=$(patsubst %.c, $(OBJDIR)/%.d, $(CFILES))
DEPFILES := $(DEPFILES_CPP) $(DEPFILES_C)

tcFarmControl: $(OBJFILES)
	$(CPP) -o $@ $^ $(LFLAGS)
	#sh archive.sh
	sh incrementBuild.sh

$(OBJDIR)/%.o: %.cpp
	$(CPP) -c -o $@ $< $(CPPFLAGS)

$(OBJDIR)/%.o: %.c 
	$(CC) -c -o $@ $< $(CCFLAGS)

#$(OBJDIR)/%.d: %.cpp
#	$(CPP) $(INCDIRS) -MM $< > $@

#$(OBJDIR)/%.d: %.c
#	$(CC) $(INCDIRS) -MM $< > $@

$(OBJDIR)/%.d: %.cpp
	$(CPP) -MM $< | tr '\n\r\\' ' ' | sed -e 's%^%$@ %' -e 's% % $(OBJDIR)/%' > $@

$(OBJDIR)/%.d: %.c
	$(CC) -MM $< | tr '\n\r\\' ' ' | sed -e 's%^%$@ %' -e 's% % $(OBJDIR)/%' > $@

#depends:
#	rm -f $(OBJDIR)/*.d
#	$(MAKE) $(DEPFILES)

run: main
	./main

clean:
	rm $(OBJDIR)/*.o
	rm $(OBJDIR)/*.d

-include $(DEPFILES)
