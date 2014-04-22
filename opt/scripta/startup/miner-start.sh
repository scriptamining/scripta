\#!/bin/bash
sudo /usr/sbin/ntpdate -u pool.ntp.org
sudo /usr/bin/screen -dmS bfgminer /opt/scripta/bin/bfgminer -c /opt/scripta/etc/miner.conf
sleep 1
echo `pidof bfgminer` > /opt/scripta/var/bfgminer.pid


