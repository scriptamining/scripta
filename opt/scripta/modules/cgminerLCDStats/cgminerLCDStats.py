#
# Simple script to read information from the cgminer API and display it on
#  the "LCD sys info" display
#
# Copyright 2013 Cardinal Communications
# If you feel this code is useful, please consider a donation to:
#  BTC address: 15aGZp2pCbpAFHcjHVrx2G746PXFo9VEed
#
# Note: A HUGE thank you goes out to Kano for is invaluable assitance with this code
#       He's a key developer on the cgminer project, and was a big help with this small project as well.
#       https://bitcointalk.org/index.php?action=profile;u=36044
#
# For more specifics about the "LCD sys info" display this code supports, see:
#  http://coldtearselectronics.wikispaces.com/USB+LCD+-+LCD+System+info
#  https://github.com/dangardner/pylcdsysinfo
#
#  To purchase one see here:
#  http://www.ebay.com/itm/USB-2-8-TFT-LCD-module-LCD-sys-info-display-temperature-fan-AIDA64-LCD-Smartie-/121004607232?pt=LH_DefaultDomain_0&hash=item1c2c6fc700
#
# NOTE: I am NOT associated in any way with ColdTears Electronics. All I can promise is that it works well for what I use it for.


import math
from pylcdsysinfo import BackgroundColours, TextColours, TextAlignment, TextLines, LCDSysInfo
import CgminerRPCClient
from optparse import OptionParser
import time
import json


#
# Config section
#
global host
global port

host = '127.0.0.1' # change if accessing cgminer instance on another box in local network
# host = '192.168.1.111' 
port = 4028     # default port - change if your is different

screenRefreshDelay = 30 # number of seconds to wait before each screen refresh (aprox.) - value overridden by command line parm
errorRefreshDelay = 30 # number of seconds to wait before each ERROR screen refresh (aprox.)
simpleDisplay = False # value overridden by command line parm

#
# call cgminer "notify" API to check for device not well error
# return: 'Well' if device error not detected
# function throws exception if hardware error is found
#
def getDeviceWellStatus(notification):
    output=""
   
    if(notification['STATUS'][0]['STATUS'] == 'S'): # all OK
        #print "hardware status ok"
        output = "Well"
        return output
    
    elif (notification['STATUS'][0]['STATUS'] == 'E'): # hardware error
        print "hardware device error detected:"
        output = str(notification['STATUS'][0]['Msg'])
        print str(notification['STATUS'][0]['Msg']) # TODO conditional log
        raise Exception(str(output)) # go to error screen
     
    else:
        output = "Uknown Device Error Detected"
        raise Exception("Uknown Device Error Detected")# let the exception handler display error screen

    return output # should never be executed
    
# END getDeviceWellStatus()
    

#
# call cgminer "pools" API to get status
# returns: URL of connected pool if found. Empty string if no pool URL found (impossible case?)
#
def getMinerPoolStatusURL():

    poolURL = ""
    allPools = []    
    
    result = CgminerRPCClient.command('pools', host, port)   
    
    if result:
        for items in result: # iterate over entire result to find POOLS collection
            if items == "POOLS":
                for i in result[items]: # found POOLS collection - build list of all pools
                    #print json.dumps(i, sort_keys=True, indent=4, separators=(',', ': ')) 
                    allPools.append(i) 
                    
    # iterate over list of pools till we find the active one, then get the URL
    for thisPool in allPools:
        if thisPool['Stratum Active'] == True and thisPool['Status'] == 'Alive':
            poolURL = thisPool['Stratum URL']
        
    return poolURL
     
# END getMinerPoolStatusURL()


#
## call cgminer "summary" API
# returns: json "summary" call results
#  Throws exception if summary is emmpty
# 
def getMinerSummary():
    result = CgminerRPCClient.command('summary', host, port)
    if result:
        #print json.dumps(result, sort_keys=True, indent=4, separators=(',', ': '))
        return result
    else:
        print "No summary data found"
        raise Exception("No summary data found") # let the exception handler display error screen
 
# END getMinerSummary()

#
## call cgminer "notify" API
# returns: json "notify" call results
#  Throws exception if notify is empty
# 
def getMinerNotifications():
    result = CgminerRPCClient.command('notify', host, port)
    if result:
        #print json.dumps(result, sort_keys=True, indent=4, separators=(',', ': '))
        return result
    else:
        print "No notify data found"
        raise Exception ("API Error - No notify data found") # let the exception handler display error screen
 
# END getMinerNotifications()

#
## call cgminer "stats" API
# returns: json "stats" call results
#  Throws exception if notify is empty
# 
def getMinerStats():
    result = CgminerRPCClient.command('stats', host, port)
    if result:
        #print json.dumps(result, sort_keys=True, indent=4, separators=(',', ': '))
        return result
    else:
        print "No stats data found"
        raise Exception("No stats data found") # let the exception handler display error screen
 
# END getMinerNotifications()

#
# call cgminer "STATS" API to get uptime
#
def getMinerPoolUptime(stats):
        output = ""
        if stats:
            uptime = stats['STATS'][0]['Elapsed']
            output = '%02d:%02d:%02d\n' % (uptime / 3600, (uptime / 60) % 60, uptime % 60)
        return output
        
# END getMinerPoolUptime()

#
# Display simplified status info screen
#
def showSimplifiedScreen(firstTime, summary):

    # extract just the data we want from the API result
    hardwareErrors = str(summary['SUMMARY'][0]['Hardware Errors'])
    avg = int(summary['SUMMARY'][0]['MHS av'])
    avgStr = convertSize(avg*1000000.0)
    avgMhs = "Average: " + avgStr
    
    # set up to write to the LCD screen
    #
    # Init the LCD screen
    display = LCDSysInfo()
    display.dim_when_idle(False)
    display.set_brightness(255)
    display.save_brightness(100, 255) 
    
    if (firstTime == True):
        display.clear_lines(TextLines.ALL, BackgroundColours.BLACK)

    display.display_text_on_line(1, str(poolURL), True, (TextAlignment.LEFT), TextColours.LIGHT_BLUE)
    display.display_text_on_line(2, "Uptime: \t" + upTime, True, (TextAlignment.LEFT, TextAlignment.RIGHT), TextColours.LIGHT_BLUE)
    display.display_text_on_line(3, avgMhs + "h/s", True, TextAlignment.LEFT, TextColours.LIGHT_BLUE)
    display.display_text_on_line(4, "HW Errors: " + hardwareErrors, True, TextAlignment.LEFT, TextColours.LIGHT_BLUE)

# END showSimplifiedScreen()


#
# Display error screen 
# (do lazy error handling - code needs to be refactored to remove API calls 
#   from display functions for better error handling display)
#
def displayErrorScreen(e):
      
    # set up to write to the LCD screen
    #
    # Init the LCD screen
    display = LCDSysInfo()
    display.dim_when_idle(False)
    display.set_brightness(255)
    display.save_brightness(100, 255)
    
    # Always clear the whole screen
    display.clear_lines(TextLines.ALL, BackgroundColours.BLACK)
    display.display_text_on_line(3, "Error: Check Miner", True, (TextAlignment.LEFT), TextColours.RED)
    display.display_text_on_line(4, e, True, (TextAlignment.LEFT), TextColours.RED)
    
# END displayErrorScreen()


def convertSize(size):
    try:
        size_name = ("k", "M", "G", "T", "P", "E", "Z", "Y")
        i = int(math.floor(math.log(size,1000)))
        p = math.pow(1000,i)
        s = round(size/p,1)

        if (s > 0):
            return '%s%s' % (s,size_name[i-1])
        else:
            return '0' 
        
    # swallow any math exceptions and just return 0 M
    except Exception as e:
        # TODO conditional log real error
        #print str(e)
        return '0'

# END convertSize(size)

#
# Display default status info screen (mimics cgminer text display where possible)
#  NOTE: screen design courtesy of "Kano". Thanks man!
#
def showDefaultScreen(firstTime, summary):

    # extract just the data we want from the API result and
    #  build up display strings for each using the data
        
    avg = float(summary['SUMMARY'][0]['MHS av'])
    avgMhs = convertSize(avg*1000000.0)
    foundBlocks = str(int(summary['SUMMARY'][0]['Found Blocks']))    
    difficultyAccepted = "A:" + str(int(summary['SUMMARY'][0]['Difficulty Accepted']))
    if 'Pool Rejected%' in summary['SUMMARY'][0]:
        rej = str(summary['SUMMARY'][0]['Pool Rejected%'])
        if (rej == '0'):
            rejp = rej + "%"
        else:
            rejp = rej.split('.')[0] + "." + rej.split('.')[1][:2] + "%"
    else:
        rejp = str(int(summary['SUMMARY'][0]['Difficulty Rejected']))
    reject = "R:" + rejp
    if 'Device Hardware%' in summary['SUMMARY'][0]:
        hw = str(summary['SUMMARY'][0]['Device Hardware%'])
        if (hw == '0'):
            hwp = hw + "%"
        else:
            hwp = hw.split('.')[0] + "." + hw.split('.')[1][:2] + "%"
    else:
        hwp = str(int(summary['SUMMARY'][0]['Hardware Errors']))
    hardware = "HW:" + hwp
    bestShare = "S:" + convertSize(int(summary['SUMMARY'][0]['Best Share']))
    workUtility = "WU:" + str(summary['SUMMARY'][0]['Work Utility']) + "/m"
   
    # get current time, and format it per user selection
    theTime = ""   
    time.ctime() # formatted like this: 'Mon Oct 18 13:35:29 2010'
    if timeDisplayFormat == '12':
        theTime = time.strftime("%I:%M%p")  # 12 hour display
    else:    
        theTime = time.strftime("%H:%M:%S")  # default to 24 hour display

    # strip common prefixes and suffixes off of the pool URL (to save display space)
    commonStringPattern = ['stratum+tcp://', 'stratum.', 'www.', '.com', 'mining.', ':3333', ':3334']  
    shortPoolURL = str(poolURL)
    for i in commonStringPattern:
        shortPoolURL = shortPoolURL.replace(i, '', 1).rstrip()   
      
    # build the display strings
    line1String = shortPoolURL + "\t" + theTime
    line2String = "Uptime:  " + upTime
    line3String = "Avg:" + avgMhs + "h/s" + "  B:" + foundBlocks
    #line3String = "Avg:" + avgMhs + "\tB:" + foundBlocks
    line4String = difficultyAccepted + "  " + bestShare
    line5String = reject + "  " + hardware
    line6String = workUtility
        
    # set up to write to the LCD screen
    #
    # Init the LCD screen
    display = LCDSysInfo()
    display.dim_when_idle(False)
    display.set_brightness(255)
    display.save_brightness(100, 255)
    
    if (firstTime == True):
        # clear screen
        display.clear_lines(TextLines.ALL, BackgroundColours.BLACK)

    # write all lines
    display.display_text_on_line(1, line1String, True, (TextAlignment.LEFT, TextAlignment.RIGHT), TextColours.YELLOW)
    display.display_text_on_line(2, line2String, True, (TextAlignment.LEFT, TextAlignment.RIGHT), TextColours.LIGHT_BLUE)    
    display.display_text_on_line(3, line3String, True, (TextAlignment.LEFT), TextColours.GREEN)
    display.display_text_on_line(4, line4String, True, (TextAlignment.LEFT), TextColours.GREEN)
    display.display_text_on_line(5, line5String, True, (TextAlignment.LEFT), TextColours.GREEN)
    display.display_text_on_line(6, line6String, True, (TextAlignment.LEFT), TextColours.GREEN)
    
    
# END showDefaultScreen()


#
## main body of application
#

if __name__ == "__main__":
    
    # print welcome message
    print "Welcome to cgminerLCDStats"
    print "Copyright 2013 Cardinal Communications"
    
    firstTime = True
    
    while(True):
        # parse the command line parms, if any
        usage = "usage: %prog [options] arg"  # set up parser object for use
        parser = OptionParser(usage)

        # setup command line options and help
        parser.add_option("-s", "--simple", action="store_true", dest="simpleDisplay", default=False, help="Show simple display layout instead of default")
        parser.add_option("-d", "--refresh-delay", type="int", dest="refreshDelay", default=3, help="REFRESHDELAY = Time delay between screen/API refresh") 
        parser.add_option("-i", "--host", type="str", dest="host", default=host, help="I.P. Address of cgminer API host")
        parser.add_option("-c", "--clock", type="str", dest="timeDisplayFormat", default='12', help="Clock Display 12 hr / 24 hr")

        # parse the arguments - stick the results in the simpleDisplay and screenRefreshDelay variables
        (options, args) = parser.parse_args()    
        simpleDisplay = options.simpleDisplay
        screenRefreshDelay = int(options.refreshDelay)
        host = options.host
        errorRefreshDelay = screenRefreshDelay
        timeDisplayFormat = options.timeDisplayFormat
        
        try:
            notification = getMinerNotifications()

            summary = getMinerSummary()

            avg = int(summary['SUMMARY'][0]['MHS av'])

            wellStatus = getDeviceWellStatus(notification)

            poolURL = getMinerPoolStatusURL()

            stats = getMinerStats()

            upTime = getMinerPoolUptime(stats)

            # display selected screen if command line option present
            if simpleDisplay:
                showSimplifiedScreen(firstTime, summary)
            else:
                showDefaultScreen(firstTime, summary) 

            firstTime = False

            time.sleep(int(screenRefreshDelay)) # Number of seconds to wait, aprox.


        except Exception as e:
            print "Main Exception Handler: "
            print str(e)
            print
            displayErrorScreen(str(e))
            time.sleep(errorRefreshDelay)






