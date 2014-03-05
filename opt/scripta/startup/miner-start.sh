#!/bin/bash
sudo /usr/sbin/ntpdate -u pool.ntp.org
sudo /usr/bin/screen -dmS cgminer /opt/scripta/bin/cgminer -c /opt/scripta/etc/miner.conf

#sudo /opt/scripta/bin/cgminer -c /opt/scripta/etc/miner.conf >> /dev/null &
#echo $! > /opt/scripta/var/cgminer.lock


