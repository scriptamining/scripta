#!/usr/bin/python

from PiMinerInfo import PiMinerInfo
from Adafruit_CharLCDPlate import Adafruit_CharLCDPlate

class PiMinerDisplay:
	
	col = []	
	prevCol = 0  
	lcd = Adafruit_CharLCDPlate()
	info = None
	mode = 1
	offset = 0
	maxOffset = 0
	screen = []	
	
	def __init__(self):
		self.lcd.clear()
		self.col = (self.lcd.ON,   self.lcd.OFF, self.lcd.YELLOW, self.lcd.OFF,
	                self.lcd.GREEN, self.lcd.OFF, self.lcd.TEAL,   self.lcd.OFF,
        	        self.lcd.BLUE,  self.lcd.OFF, self.lcd.VIOLET, self.lcd.OFF,
                	self.lcd.RED,    self.lcd.OFF)
		self.lcd.backlight(self.col[self.prevCol])
	
	#Show initial info (call after network connected)
	def initInfo(self):
		self.info = PiMinerInfo()
		self.dispLocalInfo()
		
	#Display Local Info - Accepted, Rejected, HW Errors \n Average Hashrate
	def dispLocalInfo(self):
		self.dispScreen(self.info.screen1)

	#Display Pool Name \n Remote hashrate
	def dispPoolInfo(self):
		self.dispScreen(self.info.screen2)

	#Display Rewards (confirmed + unconfirmed) \n Current Hash
	def dispRewardsInfo(self):
        	self.dispScreen(self.info.screen3)

	#Display Error rate & Uptime
	def dispUptimeInfo(self):
        	self.dispScreen(self.info.screen4)
        	
    #Display rewards & price
	def dispValueInfo(self):
        	self.dispScreen(self.info.screen5)
	
	#Send text to display
	def dispScreen(self, newScreen):
		self.screen = newScreen
		try:
			self.maxOffset = max((len(self.screen[0]) - 16), (len(self.screen[1]) - 16))
			self.lcd.clear()
			s = self.screen[0] + '\n' + self.screen[1]
			self.lcd.message(s)
		except TypeError:
			self.lcd.clear()
			self.lcd.message('connecting\nto cgminer ...')
        	

	#Cycle Backlight Color / On/Off
	def backlightStep(self):
		if self.prevCol is (len(self.col) -1): self.prevCol = -1
          	newCol = self.prevCol + 1
          	self.lcd.backlight(self.col[newCol])
          	self.prevCol = newCol
	
	#Offset text to the right
	def scrollLeft(self):
		if self.offset >= self.maxOffset: return
		self.lcd.scrollDisplayLeft()
		self.offset += 1
	
	#Offset text to the left
	def scrollRight(self):
		if self.offset <= 0: return
		self.lcd.scrollDisplayRight()
		self.offset -= 1
	
	#Display next info screen
	def modeUp(self):
		self.mode += 1
		if self.mode > 4: self.mode = 0
		self.update()
	
	#Display previous info screen
	def modeDown(self):
		self.mode -= 1
                if self.mode < 0: self.mode = 4
                self.update()
	
	#Update display
	def update(self):
		self.info.refresh()
                if self.mode == 0: self.dispPoolInfo()
                elif self.mode == 1: self.dispLocalInfo()
                elif self.mode == 2: self.dispRewardsInfo()
                elif self.mode == 3: self.dispUptimeInfo()
                elif self.mode == 4: self.dispValueInfo()
