#!/bin/bash
/usr/bin/screen -S bfgminer -X quit
cd /
git pull
sudo cp -Ru /scripta/etc/rc.local /etc/rc.local
sudo cp -Ru /scripta/opt/scripta/ /opt/
sudo cp -Ru /scripta/var/ /
sudo cp -Ru /scripta/update.sh /update.sh
sudo chown -R www-data:www-data /opt/scripta
sudo chown -R www-data:www-data /var/www
sudo chmod -R 775 /opt/scripta/startup/*.sh
sudo chmod -R 775 /etc/rc.local
sleep 5
sudo reboot
