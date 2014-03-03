---== Scripta ==---
The turnkey solution for litecoin mining with raspberry pi and fpga/asic boards

---===                                        ===---  
---=== INSTALL INTRUCTIONS ===---
---===                                        ===---

---=== The easy way ===---
1) Download the full image here 
2) Burn it on a ssd in your favourite way
3) Log in as root from a console (pw is "scripta") 
4) Remember to change root password with passwd 
5) Enjoy


---=== The way of the turtle ===---

Start from a fresh raspbian wheezy (2014-01-07)

$>sudo apt-get update

$>sudo apt-get install lighttpd

Pay attention to packet's order
$>sudo apt-get install php5-common php5-cgi php5 

$>sudo lighty-enable-mod fastcgi-php
$>sudo /etc/init.d/lighttpd force-reload
$>sudo chown www-data:www-data /var/www
$>sudo chmod 775 /var/www

---= Add pi user to www-data group =---
$>sudousermod -a -G www-data pi 


$>sudo apt-get install php5-rrd
 
$>sudo apt-get install libexpect-php5 php-auth-sasl php-mail php-net-smtp php-net-socket

---= Needed to enable https =---
$>sudo mkdir /etc/lighttpd/certs
$>sudo su
$>cd /etc/lighttpd/certs
$>openssl req -new -x509 -keyout lighttpd.pem -out 

lighttpd.pem -days 365 -nodes
$>chmod 400 lighttpd.pem

---= edit /etc/lighttpd/lighttpd.conf       =---
---= and add the following lines at the end =---
 
$SERVER["socket"] == ":443" {
  ssl.engine = "enable" 
  ssl.pemfile = "/etc/lighttpd/certs/lighttpd.pem" 
}


apt-get install libjansson4
apt-get install libusb-1.0-0
apt-get install ntpdate
apt-get install screen
