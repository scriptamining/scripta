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
    root password: scripta
    web password: scripta
    
4. Change locales to en_US

5. Add wifi support (wlan0)
    * set ssid and psk in 'network' block at '/etc/wpa_supplicant/wpa_supplicant.conf'

6. Build GridSeed GS3355 specific version of cgminer with mulit-frequency support from [repo](https://github.com/girnyau/cgminer-gc3355)
    * modified cgminer-gc3355 to report Frequency and Serial in API calls: [repo](https://github.com/mox235/cgminer-gc3355)

7. Edit '/opt/scripta/startup/miner-start.sh' to use cgminer-gc3355

8. Remove Scripta LTC donation address.  Disable MinePeon BTC hash time donation option.  Need to determine the right thing to do since _most_ of the web interface stuff is directly from the [MinePeonUI project](https://github.com/MineForeman/zArchive-MinePeonWebUI.git)
    * other Scripta rebranded PI images at [Crypto Pros](http://www.cryptopros.com/2014/03/gridseed-dual-miner-first-look-amazing.html) 
    * and [Hash Master](https://hash-master.com/blog/using-your-raspberry-pi-as-a-gridseed-mining-controller/)
    * different PI web-based controller for Gridseed at [Hashra](https://github.com/HASHRA)

9. Fix pool URL JSON encoding.  Add back miner config name/values settings from MinePeonUI.  All cgminer settings can be changed or added from miner form. 
    * priority used to reorder pool list
    * ability to set per GSD frequency based on Serial # from freq list: 
    ```
        static const int opt_frequency[] = {
            700,  706,  713,  719,  725,  731,  738,  744,
            750,  756,  763,  769,  775,  781,  788,  794,
            800,  813,  825,  838,  850,  863,  875,  888,
            900,  913,  925,  938,  950,  963,  975,  988,
           1000, 1013, 1025, 1038, 1050, 1063, 1075, 1088,
           1100, 1113, 1125, 1138, 1150, 1163, 1175, 1188,
           1200, 1213, 1225, 1238, 1250, 1263, 1275, 1288,
           1300, 1313, 1325, 1338, 1350, 1363, 1375, 1388,
           1400,
             -1
        };
    ```
10. Modify Status table
    * show KHs instead of MHs
    * replace Name with GSD Serial Number
    * replace Temperature with Frequency
    * remove percentages for DiffAccept and DiffReject 
    
11. Open Issues
    * miner commands seem broken   
    * maybe reported [hashrate](http://cryptomining-blog.com/1760-what-is-the-actual-hashrate-you-get-from-your-gridseed-asic/) is not quite accurate
    * something wierd with system time display, timezone, day-light savings

mega [scripta-20140330](https://mega.co.nz/#!D5RiSZTR!wcDqC3yOeUrYC6tqYM7Lh5YbRjVpdtQhg29CagL4ZsI)
