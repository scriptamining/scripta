#!/usr/bin/python

import sys, subprocess, time, urllib2, socket
sys.path.append("/home/pi/Adafruit-Raspberry-Pi-Python-Code/Adafruit_CharLCDPlate")
from Adafruit_CharLCDPlate import Adafruit_CharLCDPlate
from PiMinerDisplay import PiMinerDisplay

HOLD_TIME	= 3.0 #Time (seconds) to hold select button for shut down
REFRESH_TIME= 3.0 #Time (seconds) between data updates
HALT_ON_EXIT= True
display		= PiMinerDisplay()
lcd			= display.lcd
prevCol		= -1
prev		= -1
lastTime	= time.time()

def shutdown():
	lcd.clear()
	if HALT_ON_EXIT:
		lcd.message('Wait 30 seconds\nto unplug...')
		subprocess.call("sync")
		subprocess.call(["shutdown", "-h", "now"])
	else:
		exit(0)

'''
#WIP - startup on boot
def internetOn():
	try:
		response=urllib2.urlopen('http://google.com',timeout=3)
		return True
	except urllib2.URLError as err:
		pass
	return False
'''

#Check for network connection at startup
t = time.time()
while True:
	lcd.clear()
	lcd.message('checking network\nconnection ...')
	if (time.time() - t) > 120:
		# No connection reached after 2 minutes
		lcd.clear()
		lcd.message('network is\nunavailable')
		time.sleep(30)
		exit(0)
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		s.connect(('8.8.8.8', 0))
		lcd.backlight(lcd.ON)
		lcd.clear()
		lcd.message('IP address:\n' + s.getsockname()[0])
		time.sleep(5)
		display.initInfo()	# Start info gathering/display
		break         		# Success
	except:
		time.sleep(1) 		# Pause a moment, keep trying
'''
	if internetOn() == True:
		time.sleep(5)
		break         # Success
	else:
		time.sleep(1) # Pause a moment, keep trying
'''

# Listen for button presses
while True:
	b = lcd.buttons()
	if b is not prev:
		if lcd.buttonPressed(lcd.SELECT):
			tt = time.time()                        # Start time of button press
			while lcd.buttonPressed(lcd.SELECT):	# Wait for button release
				if (time.time() - tt) >= HOLD_TIME: # Extended hold?
					shutdown()						# We're outta here
			display.backlightStep()
		elif lcd.buttonPressed(lcd.LEFT):
	  		display.scrollRight()
		elif lcd.buttonPressed(lcd.RIGHT):
			display.scrollLeft()
		elif lcd.buttonPressed(lcd.UP):
			display.modeUp()
		elif lcd.buttonPressed(lcd.DOWN):
			display.modeDown()
		prev = b
		lastTime = time.time()
	else:
		now = time.time()
		since = now - lastTime
		if since > REFRESH_TIME or since < 0.0:
			display.update()
			lastTime = now
