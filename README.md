This is a new version of Scripta using BFGMiner 3.99 to support both the Round Gridseed and stick USB DualMiners. This particular version was built using the original code and some of Mox235's changes. I will post a complete image file shortly.

For both WebUI and root access the password is "scripta" (and user is root)
For DualMiner/Gridseed make sure the following options are enabled on the miner settings page:

scan - dualminer:all or gridseed:all
set-device - dualminer:clock=850 or gridseed:clock=850

If the device does not automatically recognize your miners, you can ssh root@"scripta IP address" and use command "screen -r" to manually add devices in BFG menu.

Known Issues:
1)Static Graphs not working
2)WebUI does not load if RPi is turned on with Miner's connected (to work-around this, plug in the RPi and wait until you see yellow and green activity LEDs, then wait 5-10 seconds before plugging in Miner)
3)Individual Miner Serial numbers don't show. Plan on adding support to set per-device clock speed by individual serial number.
4) Reboot and shutdown commands can be finicky.

If you have found use for this version please consider a donation, as we opted out of building it into the program

BTC: 199GzQnNAs9BBxXSmRxKECNQ1GPF2ZZ55j
LTC: LVmN9MoAbn4hQSJZrzN65oiL7W4SAY9A2q

---== Scripta ==---

The turnkey solution for litecoin mining with raspberry pi and fpga/asic boards


---===         INSTALL INTRUCTIONS            ===---



---=== The easy way ===---

1) Download the full image here http://www.lateralfactory.com/download.php?file=scripta-1_1.tgz

2) Burn it on a ssd in your favourite way

3) Log in as root from a console (pw is "scripta")

4) Remember to change root password with passwd 

5) Enjoy



---=== The way of the turtle ===---

Start from a fresh raspbian wheezy (tested with 2014-01-07) Download here http://downloads.raspberrypi.org/raspbian_latest

$>raspi-config ( if needed "Expand Filesystem" and reboot )

$>sudo apt-get update

$>sudo apt-get install lighttpd

$>sudo apt-get install php5-common php5-cgi php5 (Pay attention to packet's order)

$>sudo lighty-enable-mod fastcgi-php

$>sudo /etc/init.d/lighttpd force-reload

---= Add pi user to www-data group =---

$>sudousermod -a -G www-data pi 

$>sudo apt-get install php5-rrd libexpect-php5 php-auth-sasl php-mail php-net-smtp php-net-socket


---= Needed to enable https =---

$>sudo mkdir /etc/lighttpd/certs

$>sudo su

$>cd /etc/lighttpd/certs

$>openssl req -new -x509 -keyout lighttpd.pem -out lighttpd.pem -days 365 -nodes

$>chmod 400 lighttpd.pem

$>/etc/init.d/lighttpd force-reload


---= edit /etc/lighttpd/lighttpd.conf =---
 
$>pico /etc/lighttpd/lighttpd.conf 
 
---= add the following lines at the end =---
 
$SERVER["socket"] == ":443" {
  ssl.engine = "enable" 
  ssl.pemfile = "/etc/lighttpd/certs/lighttpd.pem" 
}

---= libs for cgminer =---

$>sudo apt-get install libjansson4 libusb-1.0-0 ntpdate screen

---= install scripta package =---

$>cd /

$>tar -xf scripta_1-1.tgz

---= point your browser on raspberry ip address, enjoy! =---
=======
All document for building USB BTC/LTC dual miner based on GC3355 chips.

1. Hardware
Include Schematic, PCB, BOM.

2. Firmware
Firmware for ARM Cortex STM32F103 CPU, to control set of GC3355 mining chips.

3. Software
cgminer and cpuminer program for BTC and LTC mining.
>>>>>>> .merge_file_P17nAC
