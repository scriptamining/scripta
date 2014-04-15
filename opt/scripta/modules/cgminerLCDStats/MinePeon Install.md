This is a quick guide to installing the cgminerLCDStats.py script on MinePeon. Note that I'm new to MinePeon as well as Arch linux for ARM. I welcome any input on this guide.

I started with a fresh install of the current version of MinePeon, v0.2.2. I took the usual steps to verify cgminer was working correctly, and that my Pi would come up on the same internal I.P. address each time.

To begin the installation, I logged in to the Pi via ssh from my main machine. I find it way easier to interact with the Pi command line over ssh, rather than logging into the Pi itself. When entering the following commands, it's easiest to copy and paste them into the terminal window. Wait for each step to complete and watch for errors. Some of the updates require user interaction, so say yes if prompted. 

Ok, let's get started. Log on to your Pi with this command:  
ssh minepeon@YOURIP    - example: ssh minepeon@192.168.1.111

Make sure the OS is up to date (Optional step - skip this is you want too, or are already on a recent version):  
`sudo pacman -Syu`

Get the "git" utility for downloading packages:  
`sudo pacman -S git`

Make sure we have all the latest MinePeon packages (Optional step - skip this is you want too, or are already on a recent version):  
`cd /opt/minepeon/`  
`git pull`  
`cd /opt/minepeon/http/`  
`git pull`  

Optional: Verify Python2 is already installed (it should be) - current version is Python 2.7.5:  
`python2 -V`

Install pyUSB library:  
`cd ~`  
`git clone https://github.com/walac/pyusb.git`  
`cd pyusb`  
`sudo python2 setup.py install`  

Install the cgminerLCDStats.py script and required modules:  
`cd ~`  
`git clone https://github.com/cardcomm/cgminerLCDStats.git`  
`cd cgminerLCDStats`  

Ok, that's it. We should be ready to go. Make sure the LCD display is connected, and let's start the script. You can start it with the default options using the following command:  
`sudo python2 cgminerLCDStats.py`

If everything went well, you should now see your cgminer stats displayed on the USB screen. Enjoy!

Note: By default, the display refreshes every 30 seconds. You can change this, and other behavior using the following command line options:

Options:
  -h, --help            show this help message and exit
  -s, --simple          Show simple display layout instead of default
  -d REFRESHDELAY, --refresh-delay=REFRESHDELAY
                        REFRESHDELAY = Time delay between screen/API refresh
  -i HOST, --host=HOST  I.P. Address of cgminer API host

