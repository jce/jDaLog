#!/bin/sh

# Simple tool for creating an source and binary archive every build. JCE, 14-6-13

currentBuild=`cat version.cpp | grep "const long tcBuildNr" | cut -d " " -f 6 | cut -d ";" -f 1`

mkdir -p archive
tar -czf archive/tcFarmControl_$currentBuild.tar.gz *.c *.cpp *.h *.sh *.txt Makefile tcFarmControl

