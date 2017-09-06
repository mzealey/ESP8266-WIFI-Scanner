A very small project inspired by [ESP8266 Mini
Sniff](https://www.hackster.io/rayburne/esp8266-mini-sniff-f6b93a) to
periodically dump MAC address and signal strength (RSSI) from the specified
WiFi network over the serial port.

It is intended to work with something like
[Find-LF])https://github.com/schollz/find-lf) to take the results from multiple
of these deployments and turn them into an internal location.

It should work in conjunction with any device that has a serial port (Raspberry
Pi, C.H.I.P etc).  I wrote a [guide on automatically reprogramming the ESP8266
via a C.H.I.P](https://markandruth.co.uk/2017/09/08/programming-esp8266-from-the-chip)
which again should work on other mini-computers.

## Compiling

The project uses [PlatformIO](http://platformio.org). Simply clone the
repository, edit the [configuration file](src/configuration.h) with your own
wireless network SSID to track, and build like:

    platformio run

You can then flash the firmware like:

    esptool.py -p /dev/ttyS2 write_flash --flash_mode dio 0 .pioenvs/esp01_1m/firmware.bin

## Dump Format

The output is printed over the serial port at 115200bps. Every second a summary
of each MAC seen over that time period is displayed preceeded by '----'. The
output format is like:

    ----
    abcdef123456 -72 1
    123456abcdef -35 4
    ----
    abcdef123456 -70 5

Where the first entry is the MAC address, the second the average (mean) RSSI,
and the third entry the number of packets that RSSI has been calculated from.

It will take a few seconds when the ESP8266 first boots to scan all wireless
networks and find the channel that your SSID is on. Once it has been found it
prints a line saying which channel it is on, and from then on every second it
produces a dump, even if no devices have been observed.
