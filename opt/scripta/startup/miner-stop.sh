#sudo kill $(cat /opt/scripta/var/cgminer.lock)

sudo /usr/bin/screen -S cgminer -X quit
