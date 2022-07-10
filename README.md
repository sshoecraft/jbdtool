
JBD BMS linux-based utility

Bluetooth support requires gattlib https://github.com/labapart/gattlib

build & install the library then edit the Makefile and set BLUETOOTH=yes

MQTT support requires the phao.mqtt.c library https://github.com/eclipse/paho.mqtt.c

build & install the library then edit the Makefile and set MQTT=yes


To build MQTT - paho.mqtt.c (https://github.com/eclipse/paho.mqtt.c.git)

        mkdir -p build && cd build
        cmake -DPAHO_BUILD_SHARED=TRUE -DPAHO_BUILD_STATIC=TRUE -DPAHO_ENABLE_TESTING=FALSE -DPAHO_BUILD_SAMPLES=FALSE ..
        make && make install

To build gattlib (https://github.com/labapart/gattlib.git)

        mkdir -p build && cd build
        cmake -DGATTLIB_BUILD_EXAMPLES=NO -DGATTLIB_SHARED_LIB=NO -DGATTLIB_BUILD_DOCS=NO -DGATTLIB_PYTHON_INTERFACE=NO ..
        make && make install

IF USING BLUETOOTH, YOU MUST PAIR THE DEVICE FIRST

        $ bluetoothctl
        # scan on
        (look for your device)
        [NEW] Device XX:XX:XX:XX:XX:XX name
        # trust XX:XX:XX:XX:XX:XX
        # pair XX:XX:XX:XX:XX:XX
        (it may ask for your passkey)


Transports are specified as:

jbdtool -t transport:target,opt1[,optN]

For CAN:

jbdtool -t can:device[,speed]

example:

	jbdtool -t can:can0,500000

For Serial:

jbdtool -t serial:device[,speed]

example:

	jbdtool -t serial:/dev/ttyS0,9600

For Bluetooth:

jbdtool -t bt:mac,descriptor

exmples:

	jbdtool -t bt:01:02:03:04:05,06

	jbdtool -t bt:01:02:03:04:05:06,ff01

For IP/esplink:

jbdtool -t ip:address[,port]

example:

	jbdtool -t ip:10.0.0.1,23

for CANServer/Can-over-ip

jbdtool -t can_ip:address,[port],interface on server,[speed]

example:

	jbdtool -t can_ip:10.0.0.1,3930,can0,500000


>>> CAN bus cannot read/write parameters


to read all parameters using bluetooth:

jbdtool -t bt:01:02:03:04:06 -r -a

to list the params the program supports, use -l

to specify single params, specify them after -r

jbdtool -t bt:01:02:03:04:06 -r BalanceStartVoltage BalanceWindow

to read a list of parameters using a file use -f:

jbdtool -t serial:/dev/ttyS0,9600 -r -f jbd_settings.fig

use -j and -J (pretty) to specify filename is a json file


to write parameters, specify a key value pair after -w

jbdtool -t ip:10.0.0.1 -w BalanceStartVoltage 4050 BalanceWindow 20


to send all output to a file, use -o.   If the filename ends with .json, file will be written in JSON format:

jbdtool -t can:can0,500000 -j -o pack_1.json
