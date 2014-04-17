#!/usr/bin/python

#This file uses code from fcicq's cgmonitor.py released under GPLv3:
#https://gist.github.com/fcicq/4975730
#for details see LICENSE.txt

import subprocess
import socket
import time
import urllib
import json
import time

class PiMinerInfo:
	
	host 		= ''
	port 		= 4028
	errRate 	= 0.0
	accepted 	= 0.0
	hw 			= 0.0
	diff1shares = 0.0
	uptime 		= ''
	screen1 	= ['no data','no data']
	screen2 	= ['no data','no data']
	screen3 	= ['no data','no data']
	screen4 	= ['no data','no data']
	screen5 	= ['no data','no data']
	currency 	= 'USD' 				#USD GBP EUR JPY AUD CAD CHF CNY DKK HKD PLN RUB SEK SGD THB NOK CZK
	dollars 	= ['USD', 'AUD', 'CAD']	#currencies with displayable symbols
	lastCheck 	= time.time()			#time of last price check
	priceWait 	= 60.0					#interval between price checks
	priceLast	= '-'					#last price via mtgox
	priceLo 	= '-'					#low price
	priceHi 	= '-'					#high price

	def __init__(self):
	  self.host = self.get_ipaddress()
	  self.refresh()
	  self.checkPrice()
	  
	def reportError(self, s):
		self.screen1 = [s, s]
		self.screen2 = [s, s]
		self.screen3 = [s, s]
		self.screen4 = [s, s]
	
	def value_split(self, s):
	  r = s.split('=')
	  if len(r) == 2: return r
	  return r[0], ''
	
	def response_split(self, s):
	  try:
		r = s.split(',')
		title = r[0]
		d = dict(map(self.value_split, r[1:]))
		return title, d
	  except ValueError:
		self.reportError('value error')
	
	def get_ipaddress(self):
		arg = 'ip route list'
		p = subprocess.Popen(arg,shell=True,stdout=subprocess.PIPE)
		data = p.communicate()
		split_data = data[0].split()
		self.ipaddr = split_data[split_data.index('src')+1]
		s = '%s' % self.ipaddr
		self.reportError(s)
		return self.ipaddr
	
	def parse_time(self, t):
		r = []
		m = t // 60
		d = 0
		if t >= 86400:
			d = t // 86400
			t = t % 86400
		r.append('%02d:%02d:%02d' % (d, t // 3600, (t % 3600) // 60))	#seconds == t % 60
		return ' '.join(r)
	
	def cg_rpc(self, host, port, command):
	  try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.connect((host, port))
		s.sendall(command)
		time.sleep(0.02)
		data = s.recv(8192)
		s.close()
	  except Exception as e:
		self.reportError(e)
		return
	  if data:
		d = data.strip('\x00|').split('|')
		return map(self.response_split, d)
	  return None
	
	def hashrate(self, h):
	  u = 'Mh/s'
	  if h >= 1000.0:
		u = 'Gh/s'
		h = h / 1000.0
	  elif h >= 1000000.0:
		u = 'Th/s'
		h = h / 1000000.0
	  s = '%s %s' % (h, u)
	  return s
	  
	def abbrev(self, v):
		v = int(v)
		if v >= 1000:
			va = float(v) / 1000.0
			vs = '%.1f' % va
			vs = vs.replace('.', 'k')
			return vs
		elif v >= 1000000:
			va = float(v) / 1000000.0
			vs = '%.1f' % va
			vs = vs.replace('.', 'm')
			return vs
		#billion
		else:
			return '%d' % v
	  
	def parse_summary(self, r):
	  if not (isinstance(r, (list, tuple)) and len(r) == 2):
		return
	  try:
		if not r[0][0] == 'STATUS=S' and r[1][0] == 'SUMMARY':
		  return
		d = r[1][1]
		self.uptime = self.parse_time(int(d['Elapsed']))
		self.accepted = float(d['Accepted'])
		self.hw = float(d['Hardware Errors'])
		try:
			#self.errRate = self.hw / self.accepted * 100.0
			self.errRate = 100.0 * self.hw / (self.diff1shares + self.hw)
		except Exception as e:
			self.errRate = 0.0
		acc = self.abbrev(d['Accepted'])
		rej = self.abbrev(d['Rejected'])
		hw = self.abbrev(d['Hardware Errors'])
		s1 = 'A:%s R:%s H:%s' % (acc, rej, hw)
		s2 = 'avg:%s' % self.hashrate(float(d['MHS av']))
		return [s1, s2]
	  except Exception as e:
		return [str(e), str(e)]

	def conv_prio_dict(self, p):
		if isinstance(p, (tuple, list, )):
			try:
				pd = dict(p)
			except TypeError:
	  			pd = zip(p, range(len(p)))
			return pd
		if isinstance(p, dict): return p
		return {}
	
	def parse_pools(self, r):
		if not isinstance(r, (list, tuple)): return ['format', 'error']
		
		try:
			if not r[0][0] == 'STATUS=S': return ['status', 'error']
			remote_time = int(r[0][1]['When'])
			for rp in r[1:]:
				if rp[0][0:4] != 'POOL': continue
	  			d = rp[1]
	  			self.diff1shares = float(d['Diff1 Shares'])
	  			if d['URL'].startswith('stratum+tcp://'): d['URL'] = d['URL'][14:]
	  			if d['URL'].startswith('http://'): d['URL'] = d['URL'][7:]
	  			d['URL'] = d['URL'].rstrip('/')
			if d['Status'] == 'Alive':
				s1 = '%s' % d['URL']
				s2 = '%s' % d['User']
				return [s1, s2]
		except Exception as e:
			return [str(e), str(e)]

	def parse_config(self, r):
		if not isinstance(r, (list, tuple)): return
		try:
			if not r[0][0] == 'STATUS=S': return
			if not r[1][0] == 'CONFIG': return
			d = r[1][1]
			return 'devcs: %s' % (int(d.get('GPU Count','0')) + int(d.get('PGA Count','0')) + int(d.get('ASC Count','0')))
		except Exception as e:
			return str(e)

	def parse_coin(self, r):
		if not isinstance(r, (list, tuple)): return
		try:
			d = r[1][1]
			return 'diff: %.2fm' % (float(d['Network Difficulty']) / 1000000.0)
		except Exception as e:
			return str(e)

	def checkPrice(self):
		try:
			url = 'https://data.mtgox.com/api/2/BTC***/money/ticker'.replace('***', self.currency)
			f = urllib.urlopen(url)
		except Exception as e:
			self.reportError(e)
			return None
		data = None
		if f:
			pricesData = f.read()
			prices_json = json.loads(pricesData)
			if prices_json and prices_json['result'] == 'success':
				data = prices_json['data']
		
		#dollar symbol currencies
		if self.currency in self.dollars:
			self.priceLast = data['last']['display_short'] if data else '-'
			self.priceLo = data['low']['display_short'] if data else '-'
			a, self.priceLo = self.priceLo.split('$')
			self.priceHi = data['high']['display_short'] if data else '-'
			a, self.priceHi = self.priceHi.split('$')
		
		#non-compatible symbol currencies
		else:
			self.priceLast = ('%.2f ' % float(data['last']['value']) if data else '-') + self.currency
			self.priceLo = '%.2f' % float(data['low']['value']) if data else '-'
			self.priceHi = '%.2f' % float(data['high']['value']) if data else '-'
	
	def refresh(self):
		
		s = self.cg_rpc(self.host, self.port, 'summary')
		self.screen1 = self.parse_summary(s)

		s = self.cg_rpc(self.host, self.port, 'pools')
		self.screen3 = self.parse_pools(s)
		
		s = self.cg_rpc(self.host, self.port, 'config')
		self.screen2[0] = self.parse_config(s)
		s = self.cg_rpc(self.host, self.port, 'coin')
		self.screen4[1] = self.parse_coin(s)
		
		self.screen4[0] = 'time: %s' % self.uptime
		self.screen2[1] = 'error: %.2f%%' % self.errRate
		
		now = time.time()
		since = now - self.lastCheck
		if since >= self.priceWait:
			self.checkPrice()
			self.lastCheck = time.time()
			
		self.screen5[0] = 'last: %s' % self.priceLast
		self.screen5[1] = 'H:' + self.priceHi + ' L:' + self.priceLo
		