# Water level

This software runs on a ESP8266 connected to a ultra sonic sensor. It reads the
measured distance and temperature from the sensor. The readings are reported
using MQTT. A webserver also runs on port 80 and shows the latest values using
the chart library [highcharts](https://www.highcharts.com/).

The ESP8266 module used for this project is an old WeMos D1 Mini.

The ultra sonic sensor is of type US-100. A serial protocol is used to read out
the values from it. The library
[PingSerial](https://github.com/stoduk/PingSerial) takes care of that.

[Platformio](https://platformio.org/) is used to build everything.

The idea is to use this software to measure the water level in a well some day.
