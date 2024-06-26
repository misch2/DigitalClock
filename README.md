![Photo of a prototype](hw/photos/prototype.jpg)

# Digital clock rework

This is a simple personal HW+SW project. My goal here was to upgrade an old JVD digital clock to automatically set the time using NTP and automatically adjust to daylight saving time. As a side effect I changed the power supply from the original 6V to a standard USB-C 5V.

Features:
 - Clock synchronized with NTP
 - Automatic daylight saving time adjustment
 - Automatic dimming

I decided to completely remove the old electronic board and replace it with a new one. While this meant losing some features such as the alarm clock, temperature monitoring, and gesture controls, it wasn't a problem for me because I hadn't used any of these for several years. The goal was just to get an on-and-forget style digital clock that wouldn't need to be constantly adjusted.

There was an 20x5 white LED matrix (with some empty positions) inside connected to a 16 (virtual columns) + 6 pin (virtual rows) header.
My idea was to connect these LEDs to the MAX7219 ICs and add ESP32 or ESP8266 as a controller and as a time source. 

This turned out to be a bit more complicated than I originally thought because while the MAX7219 can be daisy chained, each IC can only control its own matrix of 8x8 LEDs. In other words, I couldn't use the original 16x6 LED matrix and had to explicitly split it into two independent 8x6+8x6 matrixes. I used a sharp knife and some soldering to do this. Unfortunately the original circuit board didn't route the rows and columns in the most logical way, so some wire bridges had to be added and it's not very elegant in the end. But it works.

TODO add a documentation (photo?) of splitting the original PCB wiring into 2 halves.

# Software

The software is not very complex. Most of the code only takes care of mapping the logical pixels (the "virtual display") to digit and segment values for the MAX7219 ICs. I had no intention of reinventing the wheel, so I used either built-in or library functions for most tasks (such as retrieving the time from NTP or communicating with the display).

The prototype was tested on ESP32 and the production version runs on ESP8266 because I wanted to finally use it for something that doesn't require computing power.

## Display content

* `88:88:88` in full brightness
    Booting, display selftest.

* `WIFI⠀⠀⠀`
    Booting, connecting to WiFi.

* `--⠀--⠀--`
    Booting, waiting for the first NTP synchronization.

* Clock with blinking colons
    Running from NTP, time is synchronized.

* Clock without blinking colons
    Running from local time source, time has been unsynchronized for more than 6 hours.

* `OTA⠀00` up to `OTA100`
    OTA in progress (digits change from 00 % to 100 %)

* `BOOT⠀⠀`
    OTA finished, rebooting.

## Libraries used

Output from `pio run -e esp8266`:
```
Dependency Graph
|-- WiFiManager @ 2.0.17+sha.e978bc0
|-- Timemark @ 1.0.0+sha.49d4a19
|-- BH1750 @ 1.3.0+sha.b6986b5
|-- movingAvg @ 2.3.1+sha.23290e2
|-- Syslog @ 2.0.0
|-- MD_MAX72XX @ 3.5.1
|-- Wire @ 1.0
|-- ArduinoOTA @ 1.0
|-- ESP8266WiFi @ 1.0
```

