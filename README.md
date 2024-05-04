![Photo of a prototype](hw/photos/prototype.jpg)

# Digital clock rework

This is a simple personal HW+SW project. My goal here was to upgrade/fix an old JVD digital clock to automatically set time using NTP and to automatically adjust for DST. 

Features:
 - NTP synchronized clock
 - Automatic DST adjustment
 - Time based auto dimming

I decided to completely remove the old electronic board and replace it with a new one. While this meant that I'll lose some of the functions like alarm, temperature monitoring or gestures control, it wasn't an issue for me because I hadn't used any of this for several years.
So the goal was to get a "turn on and forget"-style digital clock.

There was an 20x5 white LED matrix (with some empty positions) inside connected to a 16 (virtual columns) + 6 pin (virtual rows) header.
My idea was to connect these LEDs to the MAX7219 ICs and add ESP32 or ESP8266 as a controller and as a time source. 

This came out as a bit more complicated then I originally thought because while the MAX7219 can be daisy chained each IC can only control it's own matrix of 8x8 LEDs. In other words I couldn't use the original 16x6 matrix and I had to split it into 8x6 + 8x6.
I used a sharp knife and some soldering to do this.

TODO add a documentation for splitting the original PCB wiring into 2 halves.

# Software

Software is not very complicated. Most of it just deals with mapping logical pixels to digit+segment values for MAX7219 ICs. I didn't want to play "reinvent the wheel" game so I used either built in or library functions for most of the generic tasks (like fetching the time from NTP or communicating with the display).

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

* MD_MAX72XX @ 3.1.5
* WiFiManager @ latest (2.0.17)
* Timemark @ 1.0.0
* Syslog @ 2.0.0
