#ifndef HAVE_VERSION_H
#define HAVE_VERSION_H

extern const long tcBuildNr;
//extern const long tcBuildTimestamp;

extern const char tcBuildNrText[];
//extern const char tcBuildTimestampText[];
//extern const char tcBuildTimeText[];

/*
Use this bash script in the build chain for incrementing the build number. Suggested name incrementBuild.sh

#!/bin/sh

# Simple tool for creating a build version number. JCE, 13-6-13

currentBuild=`cat version.cpp | grep "const long tcBuildNr" | cut -d " " -f 6 | cut -d ";" -f 1`
currentDateTime=`date +"%d-%m-%Y %H:%M:%S"`
currentTimestamp=`date +%s`

newBuild=`expr $currentBuild + 1`

echo "extern const long tcBuildNr = $newBuild;" > version.cpp
echo "extern const long tcBuildTimestamp = $currentTimestamp;" >> version.cpp
echo " " >> version.cpp
echo "extern const char tcBuildNrText[] = \"$newBuild\";" >> version.cpp
echo "extern const char tcBuildTimestampText[] = \"$currentTimestamp\";" >> version.cpp
echo "extern const char tcBuildTimeText[] = \"$currentDateTime\";" >> version.cpp
*/


#endif // HAVE_VERSION_H
