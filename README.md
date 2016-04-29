# MBusMqttLogger

This project is using an arduino (really, an ESP8266) to read and log data from two different meters:

Electrical meter: 
This one is measuring electrical energy consumed by my heat pump system. The meter is a simple thing that has an 
open collector output, giving one pulse for every Wh that has been consumed. A class has been created, PulsePort, 
to facilitate IRQ-based reading, and to keep track of the current value.

Heat Pump meter: 
This one is a Zenner C5 meter, providing data over an M-Bus interface (Meter Bus). A minimal implementation of the 
M-bus protocol has been implemented in the class MBus.

Featuring:
* Boots as AP/web server for configuration (if no config, or if a prog-button is pressed during start-up)
* Uses NTP library to keep track of time
* Continously stores values in EEPROM, in case of power loss or other failure (can be reset from config)
* Reports data to an MQTT broker (using json for the data)
* Can read energy as pulses using PulsePort class
* Can read energy from M-Bus using the MBus class


No further processing is done here. My OpenHAB is listening on the data being reported and do further calculations, 
storage and presentation of the data

