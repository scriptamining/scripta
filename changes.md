Inital information compiled from lots of good stuff around Scripta at litecointalk
[fourm posts](https://litecointalk.org/index.php?topic=9908.msg143787#msg143787)

1. Started with Scripta 1.1 [image](http://www.lateralfactory.com/download.php?file=scripta-1_1.tgz)
    source file in image do not match [repo]([https://github.com/scriptamining/scripta.git])
    rsync git files to image to get starting code base (all uses of "minepeon" replaced with "scripta" - why?)
    
2. Update raspberry to newest kernel (3.10.25+) and added 'slub_debug=FP' to bootline.conf
    This should maybe fix the wierd USB debug error logging issues. 
    TODO: TLS Warning (BRANCH=next rpi-update)

3. Set ssh port to 22.  Should probably turn off root ssh access and change password.
    ssh root@10.0.1.28
    password: scripta
    
4. Change locales to en_US

5. Add wifi support (wlan0)
    set ssid and psk in 'network' block at '/etc/wpa_supplicant/wpa_supplicant.conf'

6. Built GridSeed GS3355 specific version of cgminer with mulit-frequency support from [repo](https://github.com/girnyau/cgminer-gc3355)

7. Edit '/opt/scripta/startup/miner-start.sh' to use cgminer-gc3355

8. Remove Scripta LTC donation address.  Disable MinePeon BTC hash time donation option.  Need to determine the right thing to do since _all_ of the web interface stuff is directly from the [MinePeon project](http://minepeon.com/index.php/Main_Page).  

9. Issues:
    Miner commands seem broken.  
    Pool URL is sometimes wrong.  
    Replace Name with Serial number.
    Replace Freq with Temp.
    Khash instead of Mhash.
    RRD plots not updaing.  
    Add option to choose miner exe.
    Clear HW error after some time (or accept?).

10. Add GridSeed specific options to Miner form.  

{
"pools" : [
	{	
		"name" : "WeMineLTC",
		"url" : "stratum+tcp://usa.wemineltc.com:3336",
		"user" : "mox235.1",
		"pass" : "1"
	},
	{
		"name" : "WeMineLTC",
		"url" : "stratum+tcp://united.wemineltc.com:3335",
		"user" : "mox235.1",
		"pass" : "1"
	},
	{
		"name" : "ScryptGuild",
		"url" : "stratum+tcp://stratum.scryptguild.com:3333",
		"user" : "mox235_gridseed",
		"pass" : "1"
	}
],
"api-allow" : "W:127.0.0.1,192.168.1/24",
"api-listen" : true,
"failover-only" : true,
"scrypt" : true,
"gridseed-options" : "baud=115200,chips=5",
"gridseed-freq" : "6D8512995148=950,6D81116F5650=925,6D9021724857=925,6D82475A4857=900,6D973F855057=900,6D8B54835251=900"
}