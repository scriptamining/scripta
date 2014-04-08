#!/bin/bash
/usr/sbin/ntpdate -u pool.ntp.org
/usr/bin/screen -dmS cgminer /opt/scripta/bin/cgminer-gc3355 -c /opt/scripta/etc/miner.conf
sleep 1
echo `pidof cgminer-gc3355` > /opt/scripta/var/cgminer.pid