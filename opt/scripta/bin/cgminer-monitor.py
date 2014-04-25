#!/usr/bin/python
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


#
# Config
#

cgminer_host = 'localhost'
cgminer_port = 4028
email_smtp_server = 'smtp.gmail.com:587'
email_login = 'mylogin'
email_password = 'mypassword'
email_from = 'myemail@example.com'
email_to = 'myemail@example.com'
email_subject = 'Miner warning detected'
monitor_interval = 15
monitor_wait_after_email = 60
monitor_http_interface = '0.0.0.0'
monitor_http_port = 84
monitor_restart_cgminer_if_sick = True
monitor_send_email_alerts = True
monitor_max_temperature = 85
monitor_min_mhs_scrypt = 0.5
monitor_min_mhs_sha256 = 500
monitor_enable_pools = False

# MMCFE pools (www.wemineltc.com, dgc.mining-foreman.org, megacoin.miningpool.co, etc.)
# Replace the URLs and/or API keys by your own, add as many pools as you like
pools = [
    {
        'url': 'http://www.digicoinpool.com/api?api_key=1234567890',
        'cur': 'DGC'
    },
    {
        'url': 'http://www.wemineltc.com/api?api_key=1234567890',
        'cur': 'LTC'
    },
]


#
# Shared between monitor and http server
#

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
              smtpserver=email_smtp_server):
    header = 'From: %s\n' % from_addr
    header += 'To: %s\n' % ','.join(to_addr_list)
    header += 'Cc: %s\n' % ','.join(cc_addr_list)
    header += 'Subject: %s\n\n' % subject

    server = smtplib.SMTP(smtpserver)
    server.starttls()
    server.login(login, password)
    server.sendmail(from_addr, to_addr_list, header + message)
    server.quit()


#
# Monitor
#

def StartMonitor(client):
    os.system('cls')
    while (True):
        output = ''

        must_send_email = False
        must_restart = False

        result = client.command('coin', None)
        coin = ''
        if result:
            coin = result['COIN'][0]['Hash Method']
            output = 'Coin     : %s\n' % coin

        result = client.command('pools', None)
        if result:
            output += 'Pool URL : %s\n' % (result['POOLS'][0]['Stratum URL'])
            warning = ' <----- /!\\' if result['POOLS'][0]['Status'] != 'Alive' else ''
            must_send_email = True if warning != '' else must_send_email
            output += 'Pool     : %s%s\n' % (result['POOLS'][0]['Status'], warning)

        # Put this in a loop for multi-gpu support
        result = client.command('gpu', '0')
        if result:
            gpu_result = result['GPU'][0]
            warning = ' <----- /!\\' if gpu_result['Status'] != 'Alive' else ''
            must_restart = True if warning != '' else False
            must_send_email = True if warning != '' else must_send_email
            output += 'GPU 0    : %s%s\n' % (gpu_result['Status'], warning)

            min_mhs = monitor_min_mhs_scrypt if coin == 'scrypt' else monitor_min_mhs_sha256
            warning = ' <----- /!\\' if gpu_result['MHS 1s'] < min_mhs else ''
            must_send_email = True if warning != '' else must_send_email
            output += 'MHS 1s/av: %s/%s%s\n' % (gpu_result['MHS 1s'], gpu_result['MHS av'], warning)

            warning = ' <----- /!\\' if gpu_result['Temperature'] > monitor_max_temperature else ''
            must_send_email = True if warning != '' else must_send_email
            output += 'Temp     : %s%s\n' % (gpu_result['Temperature'], warning)
            output += 'Intensity: %s\n' % gpu_result['Intensity']

        result = client.command('summary', None)
        if result:
            if result['SUMMARY'][0]['Hardware Errors'] > 0:
                must_send_email = True
                output += 'HW err  : %s%s\n' % (result['SUMMARY'][0]['Hardware Errors'], ' <----- /!\\')

        result = client.command('stats', None)
        if result:
            uptime = result['STATS'][0]['Elapsed']
            output += 'Uptime   : %02d:%02d:%02d\n' % (uptime / 3600, (uptime / 60) % 60, uptime % 60)
        print output

        global shared_output
        global shared_output_lock
        shared_output_lock.acquire()
        shared_output = output
        shared_output_lock.release()

        if must_restart and monitor_restart_cgminer_if_sick:
            print 'Restarting'
            result = client.command('restart', None)

        if must_send_email and monitor_send_email_alerts and uptime > 10:
            SendEmail(from_addr=email_from, to_addr_list=[email_to], cc_addr_list=[],
                subject=email_subject,
                message=output,
                login=email_login,
                password=email_password)
            time.sleep(monitor_wait_after_email)

        # Sleep by increments of 1 second to catch the keyboard interrupt
        for i in range(monitor_interval):
            time.sleep(1)

        os.system('cls')


#
# HTTP server request handler
#

class CGMinerRequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)

        if self.path == '/favicon.ico':
            return

        self.send_header("Content-type", "text/html")
        self.end_headers()

        global shared_output
        global shared_output_lock
        shared_output_lock.acquire()
        html_output = shared_output[:-1] # one too many \n
        shared_output_lock.release()

        # Get balance from pools
        pools_output = ''
        if monitor_enable_pools:
            td_div = '<td style="padding:8px; padding-left:10%; border-top:1px solid #dddddd; background-color:#ff6600; line-height:20px;">'
            for pool in pools:
                try:
                    response = urllib2.urlopen(pool['url'])
                    data = json.load(response)
                    pools_output += '\n</td></tr>\n<tr>' + td_div + '\n' + 'pool' + '</td>' + td_div + pool['cur'] + ' %.6f' % (float(data['confirmed_rewards']))
                except urllib2.HTTPError as e:
                    pools_output += '\n</td></tr>\n<tr>' + td_div + '\n' + 'pool' + '</td>' + td_div + pool['cur'] + ' Error: ' + str(e.code)
                except urllib2.URLError as e:
                    pools_output += '\n</td></tr>\n<tr>' + td_div + '\n' + 'pool' + '</td>' + td_div + pool['cur'] + ' Error: ' + e.reason
                except:
                    pools_output += '\n</td></tr>\n<tr>' + td_div + '\n' + 'pool' + '</td>' + td_div + pool['cur'] + ' Error: unsupported pool?'

        # Format results from the monitor
        td_div = '<td style="padding:8px; padding-left:10%; border-top:1px solid #dddddd; line-height:20px;">'
        html_output = ('\n</td></tr>\n<tr>' + td_div + '\n').join(html_output.replace(': ', '</td>' + td_div).split('\n'))
        html_output += pools_output
        html = """
        <html>
            <head>
                <meta name="viewport" content="width=device-width, initial-scale=1.0">
                <title>cgminer monitor</title>
            </head>
            <body style="margin:0; font-family:Helvetica Neue,Helvetica,Arial,sans-serif;">
                <table style="vertical-align:middle; max-width:100%; width:100%; margin-bottom:20px; border-spacing:2px; border-color:gray; font-size:small; border-collapse: collapse;">
                    <tr><td style="padding:8px; padding-left:10%; border-top:1px solid #dddddd; line-height:20px;">
        """ + html_output + """
                    </td></tr>
                </table>
            </body>
        </html>
        """
        self.wfile.write(html)


#
# usage: cgminer-monitor.py [command] [parameter]
# No arguments: monitor + http server mode. Press CTRL+C to stop.
# Arguments: send the command with optional parameter and exit.
#

if __name__ == "__main__":
    command = sys.argv[1] if len(sys.argv) > 1 else None
    parameter = sys.argv[2] if len(sys.argv) > 2 else None

    client = CgminerClient(cgminer_host, cgminer_port)

    if command:
        # An argument was specified, ask cgminer and exit
        result = client.command(command, parameter)
        print result if result else 'Cannot get valid response from cgminer'
    else:
        # No argument, start the monitor and the http server
        try:
            server = SocketServer.TCPServer((monitor_http_interface, monitor_http_port), CGMinerRequestHandler)
            threading.Thread(target=server.serve_forever).start()
            StartMonitor(client)
        except KeyboardInterrupt:
            server.shutdown()

