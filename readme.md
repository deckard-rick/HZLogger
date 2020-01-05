# TGDevice

C++ library for IoT Sensors/Actors with ESP8266 (or NodeMCU etc.) made with Arduino

## Overview
There are no results by looking for base classes (or a framework) to easy implement IoT-Devices with sensors and actors with an ESP8622 (NodeMCU) on Arduino (C++).
All examples/systems I can found are not object orientated, therefor is many copy and paste needed by implementing more than one device and/or they are connected to special servers via special protocols.
In most cases the homeserver do all the managment/controlling of the devices. The better way is, that there is decentralized knowledge in each device. Therefore I dont want to use a homeserver system, but implementing my own small web based dashboard.
Web-Server and devices are connectd via http / json.

## Getting Started

Download as submodule into your project repository and install in your
Arduino library folder

## Running the tests

There are no automatic tests right now

## Deployment

compile it for your iot device

## Built With

* [Arduino](https://www.arduino.cc/) - Arduino
* [ESP8266](https://github.com/esp8266/Arduino) - ESP8266 Libraries

## Authors

* **Andreas Tengicki** - *Initial work* - [DeckardRick](https://github.com/deckard-rick)

See also the list of [contributors](https://github.com/your/project/contributors) who participated in this project.

## License

Copyright Andreas Tengicki 2018, Germany, 64347 Griesheim (tgdevice@tengicki.de)
Licence CC-BY-NC-SA 4.0, NO COMMERCIAL USE
(https://creativecommons.org/licenses/by-nc-sa/4.0/)
