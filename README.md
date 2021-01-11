
JBD BMS linux-based utility

requires MYBMM for the modules

requires gattlib https://github.com/labapart/gattlib to be installed for bluetooth

usage:

jbdtool -t <transport:target> -e <transport opts>


For CAN0:

jbdtool -t can:can0 -e 500000

For Serial:

jbdtool -t serial:/dev/ttyS0 -e 9600

For Bluetooth:

jbdtool -t bt:01:02:03:04:05:06

For IP/esplink:

jbdtool -t ip:10.0.0.1 

for CANServer/Can-over-ip

jbdtool -t can_ip:10.0.0.1 -e can0 (setting speed not working yet)


CAN bus cannot read parameters

to read all parameters using bluetooth:

jbdtool -t bt:01:02:03:04:06 -r -a

to list the params the program supports, use -l

to specify single params, specify them after -r

jbdtool -t bt:01:02:03:04:06 -r BalanceStartVoltage BalanceWindow

to read a list of parameters using a file use -f:

jbdtool -t serial:/dev/ttyS0 -e 9600 -r -f jbd_settings.fig

use -j and -J (pretty) to specify filename is a json file


to write parameters, specify a key value pair after -w

jbdtool -t ip:10.0.0.1 -w BalanceStartVoltage 4050 BalanceWindow 20


to send all output to a file, use -o.   If the filename ends with .json, file will be written in JSON format:

jbdtool -t can:can0 -e 500000 -j -o pack_1.json
