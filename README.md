Inital information compiled from lots of good stuff around Scripta at litecointalk
[fourm posts](https://litecointalk.org/index.php?topic=9908.msg143787#msg143787)

1. Started with Scripta 1.1 [image](http://www.lateralfactory.com/download.php?file=scripta-1_1.tgz)
    * source files in image do not match current Scripta [repo](https://github.com/scriptamining/scripta.git)  
    * rsync github files to image to get starting code base (all uses of "minepeon" replaced with "scripta")
    * origianl MinePeon [project](http://minepeon.com/index.php/Main_Page) has lots of good PI setup and helpful info
    
2. Update raspberry to newest kernel (3.10.25+) and added 'slub_debug=FP' to bootline.conf
    * this should fix the wierd USB debug error logging issues.   
    * TODO: TLS Warning (BRANCH=next rpi-update)  

3. Set ssh port to 22.  Should probably turn off root ssh access and change password.  
    ssh root@10.0.1.28  
    password: scripta  
    
4. Change locales to en_US

5. Add wifi support (wlan0)
    * set ssid and psk in 'network' block at '/etc/wpa_supplicant/wpa_supplicant.conf'

6. Built GridSeed GS3355 specific version of cgminer with mulit-frequency support from [repo](https://github.com/girnyau/cgminer-gc3355)
    * modified cgminer-gc3355 to report Frequency and Serial in API calls: [repo](https://github.com/mox235/cgminer-gc3355)

7. Edit '/opt/scripta/startup/miner-start.sh' to use cgminer-gc3355

8. Remove Scripta LTC donation address.  Disable MinePeon BTC hash time donation option.  Need to determine the right thing to do since _most_ of the web interface stuff is directly from the [MinePeonUI project](https://github.com/MineForeman/zArchive-MinePeonWebUI.git)
    * other Scripta rebranded PI images at [Crypto Pros](http://www.cryptopros.com/2014/03/gridseed-dual-miner-first-look-amazing.html) 
    * and [Hash Master](https://hash-master.com/blog/using-your-raspberry-pi-as-a-gridseed-mining-controller/)
    * different PI web-based controller for Gridseed at [Hashra](https://github.com/HASHRA)

9. Fix pool URL JSON encoding.  Add miner back miner config name/values settings from MinePeonUI.  All cgminer settings can be changed or added from miner form. 

10. Modify Status table
    * show KHs instead of MHs
    * replace Name with Serial Number
    * replace Temperature with Frequency
    
11. Issues
    * Miner commands seem broken   
    * Reorder pool list based on prio

