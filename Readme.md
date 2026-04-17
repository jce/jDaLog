# jDaLog, jce's Data Logger

Data logging system for time/value series from S7 PLC and Modbus.

<img src="doc/Screenshot at 2026-04-16 13-39-32.png" alt="jDaLog with compare page for multiple inputs." width="500">

What it does
- Reads data from Siemens S7 PLCs and Modbus (TCP and RTU)
- Stores data in binary format
- Serves simple HTTP user interface
- Generates plotted views of collected data

Key parts
- PLC tag batching client, based on snap7
- Modbus communication based on libmodbus
- gnuplot for plotting, draws min, average and max per time section.

## Status

Perpetual work in progress, continuously evolving since 2013. Some modules are stable; others are experimental or deprecated.

- PLC interface is stable
- Modbus interface is stable
- Mongoose web server changed to libmicrohttpd due to licensing. Not all pages work.

## Install

Required packages: (sudo apt install)

```bash
git cmake libjansson-dev libmodbus-dev gnuplot libmicrohttpd-dev libcurl4-openssl-dev
```

Optional packages:

```
p7zip p7zip-full nmap
```

Install libsnap7:
```
git clone https://github.com/davenardella/snap7.git
cd snap7/build/linux
make clean
make all
sudo make install
```

Install jDaLog
```
git clone https://github.com/jce/jDaLog.git
cd jDaLog
mkdir build
cd build
cmake ../src
make -j4
cd ..
```

Copy the config_example.json to config.json, modify as required

Start with ./start.sh

Open the url "http://localhost:8092"

## Recommended Postbuild steps:

### /tmp on ramdisk:
jDaLog uses /tmp for storing gnuplotscript files and plotted graphs. This could be best done on a ramdisk.

Add to /etc/fstab:

```
tmpfs   /tmp    tmpfs   nodev,nosuid,size=1G    0   0
```

restart machine

### Add as systemd service:

modify jDaLog.service: change username and directory path
```
sudo cp jDaLog.service /etc/systemd/system
sudo systemctl enable jDaLog.service
sudo systemctl start jDaLog.service
```
