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
