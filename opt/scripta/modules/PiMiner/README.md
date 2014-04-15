PiMiner v1.1
=======

Python scripts for interfacing cgminer with the Adafruit 16x2 LCD + Keypad Kit for Raspberry Pi.

Project tutorial: http://learn.adafruit.com/piminer-raspberry-pi-bitcoin-miner

In progress!

Adafruit invests time and resources providing this open source code, please support Adafruit and open-source hardware by purchasing products from Adafruit!

Written by Collin Cunningham for Adafruit Industries. BSD license, all text above must be included in any redistribution

To download, log into your Pi with Internet accessibility and type: git clone https://github.com/adafruit/PiMiner.git


Changes
-------------

Version 1.1
- Added mining auto-start after boot (see tutorial)
- Time format changed to dd:hh:mm
- Abbreviated large share count; ex. "1k2"
- Revised error % calculation: 100 * HW / (diff1shares + HW)
- Added MtGox last, high, & low price ("currency" var can be set in script)
- Misc. tutorial revisions


Version 1.0
- Initial release
