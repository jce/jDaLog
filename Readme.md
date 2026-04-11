jDaLog, jce's Data Logger

Data logging system for S7 PLC and modbus.

What it does
-Reads data from modbus and Siemens S7 PLCs
-Stores data in binary format
-Generates plotted views of collected data
Key parts
-Snap7 batching client for PLC reads
-Modbus communication library
-task scheduler
Status

Perpetual work in progress. Some modules are stable; others are experimental or deprecated.

Notes
-Mongoose web server removed due to licensing

*Install*

Required packages: (sudo apt install)
git cmake libjansson-dev libmodbus-dev gnuplot libmicrohttpd-dev libcurl4-openssl-dev

Optional packages:
libmariadbclient-dev libmariadb-dev-compat p7zip p7zip-full nmap libusb-dev

Install libsnap7:
git clone https://github.com/davenardella/snap7
cd snap7/build/linux
make clean
make all
sudo make install

Install jDaLog
git clone git@github.com:jce/jDaLog.git
cd jDaLog
mkdir build
cd build
cmake ../src

There are some build options: (with defaults)
HAVE_S7	(ON) Builds interface_S1200, links with libsnap7.
HAVE_RPI (OFF) Builds interface_pi_gpio and cpu temperature and frequency readout for Raspberry Pi specific.
HAVE_USB (OFF) Builds interface_k8055, links with libusb.
HAVE_MARIA (OFF) Builds with some legacy MariaDB stuff.
HAVE_LINUX_I2C (ON) Build with linux I2C support

make -j4
cd ..
Copy the config_example.json to config.json, modify as required
Start with ./jDaLog

Recommended Postbuild steps:
jDaLog uses /tmp for storing gnuplotscript files and plotted graphs. This could be best done on a ramdisk.
Add to /etc/fstab:
tmpfs   /tmp    tmpfs   nodev,nosuid,size=1G    0   0
restart machine

Add as systemd service:
modify jDaLog.service: change username and directory path
sudo cp jDaLog.service /etc/systemd/system
sudo systemctl enable tcFC.service
sudo systemctl start tcFC.service

