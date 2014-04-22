#!/usr/bin/python
#
# Scripta watchdog test for cgminer status
#
# parts used from cgminer-monitor.py
# Romain Dura | romain@shazbits.com
# https://github.com/shazbits/cgminer-monitor
# BTC 1Kcn1Hs76pbnBpBQHwdsDmr3CZYcoCAwjj
#

import socket
import sys
import time
import smtplib
import json
import os
import threading
import SimpleHTTPServer
import SocketServer
import urllib2
import datetime, time
import pprint

#
# Config
#

cgminer_host = 'localhost'
cgminer_port = 4028
monitor_interval = 15
monitor_wait_after_email = 10

shared_output = ''
shared_output_lock = threading.Lock()

#
# cgminer RPC
#

class CgminerClient:
    def __init__(self, host, port):
        self.host = host
        self.port = port

    def command(self, command, parameter):
        # sockets are one time use. open one for each command
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        try:
            sock.connect((self.host, self.port))
            if parameter:
                self._send(sock, json.dumps({"command": command, "parameter": parameter}))
            else:
                self._send(sock, json.dumps({"command": command}))
            received = self._receive(sock)
        except Exception as e:
            print e
            sock.close()
            return None

        sock.shutdown(socket.SHUT_RDWR)
        sock.close()

        # the null byte makes json decoding unhappy
        try:
            decoded = json.loads(received.replace('\x00', ''))
            return decoded
        except:
            pass # restart makes it fail, but it's ok

    def _send(self, sock, msg):
        totalsent = 0
        while totalsent < len(msg):
            sent = sock.send(msg[totalsent:])
            if sent == 0:
                raise RuntimeError("socket connection broken")
            totalsent = totalsent + sent

    def _receive(self, sock, size=65500):
        msg = ''
        while True:
            chunk = sock.recv(size)
            if chunk == '':
                # end of message
                break
            msg = msg + chunk
        return msg


#
# Utils
#

def SendEmail(from_addr, to_addr_list, cc_addr_list,
              subject, message, login, password,
              smtpserver):
    header = 'From: %s\n' % from_addr
    header += 'To: %s\n' % ','.join(to_addr_list)
    header += 'Cc: %s\n' % ','.join(cc_addr_list)
    header += 'Subject: %s\n\n' % subject

    server = smtplib.SMTP(smtpserver)
    server.starttls()
    server.login(login, password)
    server.sendmail(from_addr, to_addr_list, header + message)
    server.quit()
    print 'send email: ' + to_addr_list[0]

    
if __name__ == "__main__":

    now = (datetime.datetime.now()-datetime.datetime(1970,1,1)).total_seconds()
  
    if os.path.exists('/tmp/wdog.ts'):
      ts_file=open('/tmp/wdog.ts','r')
      dt = now - float(ts_file.readline())
      ts_file.close();
    else:
      ts_file=open('/tmp/wdog.ts','w')
      ts_file.write(str(now));
      ts_file.close;
      dt = 0.0;

    if os.path.exists('/tmp/reboot.ts'):
      print 'OK - wait for reboot command'
      sys.exit(0)
          
    conf_file=open('/opt/scripta/etc/scripta.conf')
    conf = json.load(conf_file)
    pprint.pprint(conf)
    conf_file.close()
    
    if conf['rebootEnable']:
      conf['rebootEnable'] = False
      with open('/opt/scripta/etc/scripta.conf', 'w') as outfile:
        json.dump(conf, outfile)
      print 'OK - reboot command'
      sys.exit(1)
        
    if not conf['recoverEnable']:
      print 'OK - scripta watchdog disabled'
      sys.exit(0)

    if dt < 60.0:
      print 'OK - wait for cgminer startup ' + str(dt) + ' sec'
      sys.exit(0)
      
    print 'expected device count: ' + str(conf['miningExpDev'])
    print 'expected min hashrate: ' + str(conf['miningExpHash'])

    pid_file=open('/opt/scripta/var/bfgminer.pid','r')
    if not pid_file:
      print 'ERROR - bfgminer.pid not found'
    else:
      pid = int(pid_file.readline())
      pid_file.close()
      ps = '/proc/' + str(pid) + '/'
      if os.path.exists(ps):
        print 'OK - bfgminer process running'
      else:
        output = 'ERROR - bfgminer process not running (pid ' + ps + ')'
        time.sleep(3)
        if not os.path.exists(ps):
          output = 'ERROR - bfgminer process not running (pid ' + ps + ')'
          if conf['alertEnable']:
            SendEmail(
              from_addr='scripta@hotmail.com',
              to_addr_list=[conf['alertEmailTo']],
              cc_addr_list=[],
              subject='Scripta Reboot [' + conf['alertDevice'] + ']',
              message=output,
              login=conf['alertSmtpUser'],
              password=conf['alertSmtpPwd'],
              smtpserver=conf['alertSmtp'] + ':' + str(conf['alertSmtpPort']))
            time.sleep(monitor_wait_after_email)
          ts_file=open('/tmp/reboot.ts','w')
          ts_file.write(str(now));
          ts_file.close;
          sys.exit(1) # reboot
        
    client = CgminerClient(cgminer_host, cgminer_port)

    result = client.command('pgacount', None)
    if result:
      dev = result['PGAS'][0]['Count']
      if conf['miningExpDev'] > dev:
        output = 'ERROR - device count: ' + str(dev) + '\n\n' + pprint.pformat(result)
        print output
        if conf['alertEnable']:
          SendEmail(
            from_addr='scripta@hotmail.com',
            to_addr_list=[conf['alertEmailTo']],
            cc_addr_list=[],
            subject='Scripta Reboot [' + conf['alertDevice'] + ']',
            message=output,
            login=conf['alertSmtpUser'],
            password=conf['alertSmtpPwd'],
            smtpserver=conf['alertSmtp'] + ':' + str(conf['alertSmtpPort']))
          time.sleep(monitor_wait_after_email)
        ts_file=open('/tmp/reboot.ts','w')
        ts_file.write(str(now));
        ts_file.close;
        sys.exit(1) # reboot
      else:
        print 'OK - device count: ' + str(dev)
    else:
        print 'OK - cgminer not ready'
        sys.exit(0);
        
    result = client.command('devs', None)
    if result:
      for d in result['DEVS']:
        if conf['miningExpHash'] > d['MHS rolling']:
          output = 'ERROR - ' + str(d['Name']) + str(d['ID']) + ' hashrate: ' + str(d['MHS rolling']) + '\n\n' + pprint.pformat(result)
          print output
          if conf['alertEnable']:
            SendEmail(
              from_addr='scripta@hotmail.com',
              to_addr_list=[conf['alertEmailTo']],
              cc_addr_list=[],
              subject='Scripta Reboot [' + conf['alertDevice'] + ']',
              message=output,
              login=conf['alertSmtpUser'],
              password=conf['alertSmtpPwd'],
              smtpserver=conf['alertSmtp'] + ':' + str(conf['alertSmtpPort']))
            time.sleep(monitor_wait_after_email)
          ts_file=open('/tmp/reboot.ts','w')
          ts_file.write(str(now));
          ts_file.close;
          sys.exit(1) # reboot
        else:
          print 'OK - ' + str(d['ID']) + ' hashrate: ' + str(d['MHS rolling'])
              
    sys.exit(0)
