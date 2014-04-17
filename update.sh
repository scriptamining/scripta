#!/bin/bash
/usr/bin/screen -S bfgminer -X quit
cd
git pull
sudo chown -R www-data /opt/scripta
sudo chown -R www-data /var/www
sudo chmod -R 775 /opt/scripta/startup/*.sh
sudo chmod -R 775 /etc/rc.local
sleep 5
sudo reboot
