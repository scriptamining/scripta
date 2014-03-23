#!/bin/bash
sudo /usr/sbin/ntpdate -u pool.ntp.org
sudo /usr/bin/screen -dmS cgminer /opt/scripta/bin/cgminer-gc3355 -c /opt/scripta/etc/miner.conf
