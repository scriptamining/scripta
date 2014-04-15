# -*- coding: UTF-8 -*-
#
# Interface with LCD Sys info USB device
#
# Copyright (C) 2012  Dan Gardner
#
# USB initialisation code has been adapted from pywws by Jim Easterbrook
# display_{text,icon}_anywhere by Manuel Mausz
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# See <http://www.gnu.org/licenses/gpl-3.0.txt>

import time
import struct

try:
    import usb
except ImportError:
    # caller may not want usb access, e.g. image conversion
    usb = None

# Use Semantic Versioning, http://semver.org/
version_info = (0, 0, 1, 'a1')
__version__ = '.'.join(map(str, version_info))

try:
    from PIL import Image  # http://www.pythonware.com/products/pil/
except ImportError:
    try:
        import Image  # http://www.pythonware.com/products/pil/
    except ImportError:
        Image = None


_font_length_table = [
    0x11, 0x06, 0x08, 0x15, 0x0E, 0x19, 0x15, 0x03, 0x08, 0x08, 0x0F, 0x0D,
    0x05, 0x08, 0x06, 0x0B, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x06, 0x06, 0x13, 0x10, 0x13, 0x0C, 0x1A, 0x14, 0x10, 0x12,
    0x13, 0x0F, 0x0D, 0x13, 0x11, 0x04, 0x07, 0x11, 0x0E, 0x14, 0x11, 0x15,
    0x0F, 0x15, 0x12, 0x10, 0x13, 0x11, 0x14, 0x1C, 0x13, 0x13, 0x12, 0x07,
    0x0B, 0x07, 0x0B, 0x02, 0x08, 0x0E, 0x0F, 0x0E, 0x0F, 0x10, 0x0B, 0x0F,
    0x0E, 0x04, 0x07, 0x0F, 0x04, 0x18, 0x0E, 0x10, 0x0F, 0x0F, 0x0A, 0x0D,
    0x0B, 0x0E, 0x10, 0x16, 0x10, 0x10, 0x0E, 0x01, 0x11, 0x02
]

large_image_indexes = [x * 38 + 180 for x in range(0,8)]

# Padding necessary for left-aligned text in the right-hand column.
COL2LEFT = '|' * 9 + '___'

def count_bits_set(field):
    """Count the number of bits set in an integer

    Discovered independently by:

      - Brian W. Kernighan (C Programming Language 2nd Ed.)
      - Peter Wegner (CACM 3 (1960), 322)
      - Derrick Lehmer (published in 1964 in a book edited by Beckenbach)

    Source: http://www-graphics.stanford.edu/~seander/bithacks.html
    """
    count = 0
    while field:
        field &= field - 1
        count += 1
    return count

class TextColours(object):
    """Colour palette for text colours"""
    GREEN       = 1
    YELLOW      = 2
    RED         = 3
    WHITE       = 5
    CYAN        = 6
    GREY        = 7
    BLACK       = 13
    BROWN       = 15
    BRICK_RED   = 16
    DARK_BLUE   = 17
    LIGHT_BLUE  = 18
    ORANGE      = 21
    PURPLE      = 22
    PINK        = 23
    PEACH       = 24
    GOLD        = 25
    LAVENDER    = 26
    ORANGE_RED  = 27
    MAGENTA     = 28
    NAVY        = 30
    LIGHT_GREEN = 31

class BackgroundColours(object):
    """Palette of 16-bit (highcolor) background colours"""
    BLACK       = 0x0000
    BLUE        = 0x001f
    GREEN       = 0x07e0
    CYAN        = 0x07ff
    BROWN       = 0x79e0
    DARK_GREY   = 0x7bef
    LIGHT_GREY  = 0xbdf7
    RED         = 0xf800
    PURPLE      = 0xf81f
    ORANGE      = 0xfbe0
    GOLD        = 0xfd20
    YELLOW      = 0xffe0
    WHITE       = 0xffff

class TextAlignment(object):
    """Text alignment specifiers"""
    NONE        = -1
    CENTRE      = 0
    LEFT        = 1
    RIGHT       = 2

class TextLines(object):
    """Text line specifiers"""
    LINE_1      = 1 << 0
    LINE_2      = 1 << 1
    LINE_3      = 1 << 2
    LINE_4      = 1 << 3
    LINE_5      = 1 << 4
    LINE_6      = 1 << 5
    ALL         = LINE_1 + LINE_2 + LINE_3 + LINE_4 + LINE_5 + LINE_6


def _le_unpack(byte):
    """Converts little-endian byte string to integer."""
    return sum([ b << (8 * i) for i, b in enumerate(byte) ])

def _bmp_to_raw(bmpfile):
    """Converts a 16bpp, RGB 5:6:5 bitmap to a raw format bytearray."""
    data_offset = _le_unpack(bytearray(bmpfile[0x0a:0x0d]))
    width = _le_unpack(bytearray(bmpfile[0x12:0x15]))
    height = _le_unpack(bytearray(bmpfile[0x16:0x19]))
    bpp = _le_unpack(bytearray(bmpfile[0x1c:0x1d]))

    if bpp != 16:
        raise IOError("Image is not 16bpp")

    if (width != 36 or height != 36) and (width != 320 or height != 240):
        raise IOError("Image dimensions must be 36x36 or 320x240 (not %dx%d)" % (width, height))

    raw_size = width * height * 2
    rawfile = bytearray(b'\x00' * (raw_size + 8))
    rawfile[0:8] = [16, 16, bmpfile[0x13], bmpfile[0x12], bmpfile[0x17], bmpfile[0x16], 1, 27]
    raw_index = 8

    for y in range(0, height):
        current_index = (width * (height - (y + 1)) * 2) + data_offset
        for k in range(0, width):
            rawfile[raw_index] = bmpfile[current_index + 1]
            rawfile[raw_index + 1] = bmpfile[current_index]
            raw_index += 2
            current_index += 2

    return rawfile

def raw_to_image(data):
    """Convert raw image data, as returned from _bmp_to_raw(), into PIL Image.
    Raw data is big endian 16 bit rgb565 format with a custom 8 byte header.
    """
    be_ui2 = '>H'
    header = data[0:8]
    """Image header details
    print repr(header)
    for c, d in enumerate(header):
        d = ord(d)
        print '%d  0x%02x %d' % (c, d, d)
    
        '\x10\x10\x01@\x00\xf0\x01\x1b'
    Index  Hex  Decimal   Description
        0  0x10 16      # FIXED
        1  0x10 16      # FIXED
        2  0x01 1       # width byte 1
        3  0x40 64      # width byte 2
        4  0x00 0       # height byte 1
        5  0xf0 240     # height byte 2
        6  0x01 1       # FIXED
        7  0x1b 27      # FIXED
    """
    width = header[2:2 + 2]
    width = struct.unpack(be_ui2, width)[0]
    height = header[4:4 + 2]
    height = struct.unpack(be_ui2, height)[0]
    
    raw_data = data[8:]  # 16 bit rgb565 big endian buffer
    """
    im = Image.frombuffer('RGB', (width, height), raw_data, 'raw', 'RGB', 0, 1)
    """
    im = Image.new("RGB", (width, height))
    for y in xrange(height):
        current_index = (width * (height - (y + 1)) * 2)
        y_dx = (height - y) - 1
        for x in xrange(width):
            px1 = raw_data[current_index]
            px2 = raw_data[current_index + 1]
            
            # convert big-endian 2 bytes into short
            #px = ord(px2) + (ord(px1) * 256)  # dumb big endian 2 bytes to short
            px = struct.unpack(be_ui2, raw_data[current_index:current_index + 2])[0]
            
            # Convert from rgb565 into rgb888
            im.putpixel((x, y_dx),
                    (
                        (px & 0xF800) >> 8,
                        (px & 0x07E0) >> 3,
                        (px & 0x001F) << 3
                    )
                )
            current_index += 2
    im.save('test.png')
    return im

def image_to_raw(im):
    """Convert PIL Image into raw image data, reverse of raw_to_image.
    This is pretty much a straight clone of the approach taken by _bmp_to_raw()
    this is not efficient"""
    be_ui2 = '>H'

    im = im.convert('RGB')  # convert to rgb888
    width, height = im.size
    width_bin = struct.pack(be_ui2, width)
    height_bin = struct.pack(be_ui2, height)
    
    raw_size = width * height * 2
    rawfile = bytearray(b'\x00' * (raw_size + 8))
    rawfile[0:8] = [16, 16, width_bin[0], width_bin[1], height_bin[0], height_bin[1], 1, 27]
    raw_index = 8

    for y in xrange(height):
        current_index = raw_index + (width * (height - (y + 1)) * 2)
        y_dx = (height - y) - 1
        for x in xrange(width):
            r, g, b = im.getpixel((x, y_dx))
            
            rgb565 = (int(float(r) / 255 * 31) << 11) | (int(float(g) / 255 * 63) << 5) | (int(float(b) / 255 * 31))
            """
            r_new = (r >> 3) << 3
            g_new = (g >> 2) << 2
            b_new = (b >> 3) << 3

            print x, y, y_dx, (r, g, b), (hex(r), hex(g), hex(b)), (hex(r_new), hex(g_new), hex(b_new))
            #print rgb565, hex(rgb565)
            """
            one_pixel = struct.pack(be_ui2, rgb565)
            rawfile[current_index] = one_pixel[0]
            rawfile[current_index + 1] = one_pixel[1]
            current_index += 2
    return rawfile

MAX_IMAGE_SIZE = (320, 240)  # LCD screen size

def simpleimage_resize(im, expected_size=MAX_IMAGE_SIZE):
    if im.size > expected_size:
        """resize - maintain aspect ratio
        NOTE PIL thumbnail method does not increase
        if new size is larger than original
        2 passes gives good speed and quality
        """
        im.thumbnail((expected_size[0] * 2, expected_size[1] * 2))
        im.thumbnail(expected_size, Image.ANTIALIAS)
    
    # image is not too big, but it may be too small
    # _may_ need to add black bar(s)
    if im.size < expected_size:
        im = im.convert('RGB')  # convert to RGB
        bg_col = (0, 0, 0)
        background = Image.new('RGB', expected_size, bg_col)
        x, y = im.size
        background.paste(im, (0, 0, x, y))  # does not center/centre
        im = background
    
    return im


class LCDSysInfo(object):

    """A Python driver for the Coldtears LCD Sys Info
    (http://coldtearselectronics.wikispaces.com/)
    """

    usb_timeout_ms = 5000
    clear_line_wait_ms = 1000
    erase_sector_wait_ms = 220
    min_display_icon_wait_ms = 10
    max_display_icon_wait_ms = 700
    write_page_wait_ms = 15
    display_sysinfo_wait_ms = 50

    # Values for calculating line-drawing times
    max_display_text_wait_ms = 85
    chars_per_icon = 2.75

    def __init__(self, index=0):
        """Opens a handle to an LCD Sys Info device.

        Args:
            index (int): The index of the device in the list of connected LCD
                Sys Info devices, with zero (the default) being the first device.
        Raises:
            IOError: An error ocurred while opening the LCD Sys Info device.
            RuntimeError: PyUSB 0.4 or later is required.
        """

        if usb is None:
            raise ImportError('No module named usb')

        dev = self._find_device(0x16c0, 0x05dc, index)
        if not dev:
            raise IOError("LCD Sys Info device not found")
        self.devh = dev.open()
        if not self.devh:
            raise IOError("Failed to open device")
        try:
            self.devh.claimInterface(0)
        except usb.USBError:
            if not hasattr(self.devh, "detachKernelDriver"):
                raise RuntimeError("Please upgrade python-usb (pyusb) to 0.4 or later")
            try:
                self.devh.detachKernelDriver(0)
                self.devh.claimInterface(0)
            except usb.USBError:
                raise IOError("Failed to claim interface")

    def __del__(self):
        """Closes the handle to the LCD Sys Info device."""
        if self.devh:
            try:
                self.devh.releaseInterface()
            except ValueError:
                # interface was not claimed, not a problem
                pass

    def _find_device(self, idVendor, idProduct, index):
        """Locate the index'th device with specified vendor and product id."""
        for bus in usb.busses():
            for dev in bus.devices:
                if dev.idVendor == idVendor and dev.idProduct == idProduct:
                    index -= 1
                    if index < 0:
                        return dev
        return None

    def set_brightness(self, value):
        """Set the brightness of the LCD backlight without saving the value to the device.

        Args:
            value (int): Number representing the LCD brightness, in the range 0 to 255.
        """
        value = max(0, min(value, 255))
        self.devh.controlMsg(0x40, 13, "", min(value, 255), min(value, 255), self.usb_timeout_ms)

    def save_brightness(self, off_value, on_value):
        """Set the brightness of the LCD backlight when idle and active and
        save the values to the device.

        Args:
            off_value (int): Number representing the LCD backlight brightness
                when the LCD is idle.
            on_value (int): Number representing the LCD backlight brightness
                when the LCD is active.
        """
        self.devh.controlMsg(0x40, 14, "", off_value + on_value * 256, 0, self.usb_timeout_ms)

    def display_icon(self, position, icon_number):
        """Display an icon at a specified position on the device.

        Args:
            position (int): Number representing the position in which to display
                the icon, in the range 0 to 47, where 0 is the top-left corner
                of the display and 48 is the bottom-right corner.
            icon_number (int): The index of the icon to be displayed. This may be
                in the range 1 to 43 for the default icons or 257+ for downloaded
                icons. An invalid icon number will display garbage to screen.
        """
        # TODO create enumeration class for icons
        position = max(0, min(position, 47))
        self.devh.controlMsg(0x40, 27, "", position * 512 + icon_number, 25600, self.usb_timeout_ms)

        if icon_number in large_image_indexes:
            time.sleep(self.max_display_icon_wait_ms / 1000.0)
        else:
            time.sleep(self.min_display_icon_wait_ms / 1000.0)

    def display_icon_anywhere(self, pos_x, pos_y, icon_number):
        """Display an icon at an exact position on the device.

         Requires firmware >=1.04

        Args:
            pos_x (int): Number representing the position on the x-axis, in the
                range 0 to 320 minus the width of your icon, where 0 is the left
                edge of the display and 320 is the right edge.
            pos_y (int): Number representing the position on the y-axis, in the
                range 0 to 240 minus the width of your icon, where 0 is the top
                edge of the display and 240 is the bottom edge.
            icon_number (int): The index of the icon to be displayed. This may be
                in the range 1 to 43 for the default icons or 257+ for downloaded
                icons. An invalid icon number will display garbage to screen.
        """
        # TODO create enumeration class for icons
        pos_x = max(0, min(pos_x, 320))
        pos_y = max(0, min(pos_y, 240))
        ba = struct.pack("<BBBB", pos_y >> 8, pos_y & 0xFF, pos_x >> 8, pos_x & 0xFF)
        tmp = (icon_number << 8) + icon_number
        self.devh.controlMsg(0x40, 29, ba, tmp, tmp, self.usb_timeout_ms)

        if icon_number in large_image_indexes:
            time.sleep(self.max_display_icon_wait_ms / 1000.0)
        else:
            time.sleep(self.min_display_icon_wait_ms / 1000.0)

    def set_text_background_colour(self, colour):
        """Set the background colour for text display.

        Args:
            colour (int): The background colour from pylcdsysinfo.BackgroundColours.
        """
        self.devh.controlMsg(0x40, 30, "", colour, 0, self.usb_timeout_ms)

    def _align_text(self, mm, alignment, screen_px, string_length_px):
        """Align text suitably for the specified screen width"""
        spaces,pixels = divmod(screen_px - string_length_px, 17)
        if alignment == TextAlignment.CENTRE:
            mm = mm.center(len(mm) + spaces, " ").center(len(mm) + pixels, "{")
        elif alignment == TextAlignment.LEFT:
            mm = mm + " " * spaces + "{" * pixels
        elif alignment == TextAlignment.RIGHT:
            mm = " " * spaces + "{" * pixels + mm
        return mm

    def _text_conversion(self, mm, field_size, alignment):
        """Pad, truncate, align and otherwise munge the specified text."""
        screen_px = (40 * field_size) - 1
        mm = mm.strip().replace(" ", "___")
        string_length_px = 0
        for k in range(0, len(mm)):
            ascii_value = ord(mm[k])
            char_length_px = 0
            if ascii_value >= 32 and ascii_value <= 125:
                char_length_px = _font_length_table[ascii_value - 32]
            if string_length_px + char_length_px > screen_px:
                mm = mm[0:k]
                break
            string_length_px += char_length_px
        return self._align_text(mm, alignment, screen_px, string_length_px)

    def display_text_on_line(self, line, text_string, pad_for_icon, alignment, colour, field_length=8):
        """Display text on a line of the device.

        Args:
            line (int): The line on which the text should be displayed, in the range 1 to 6.
            text_string (str): The text to be displayed, which will be truncated as
                required. Use "|" for wider spacing blank and "^" to display a "degrees" symbol.
                Use "\\t" (TAB) to construct a two-column layout.
                All characters should be printable 7 bit US-ASCII characters.
                Other special characters than TAB, pipe, and caret are space and underscore.
                " " at the start of a string is ignored (instead use pipe, "|").
                "_" is treated as a (very small) small blank space.
                The following characters are ignored, "{", "}", and "~".
            pad_for_icon (bool): If true, padding will be added to the left of the text,
                to accommodate an icon.
            alignment (int): The text alignment from pylcdsysinfo.TextAlignment.
                May be a list/tuple with two values if text_string contains "\\t".
            colour (int): The text colour from pylcdsysinfo.TextColours.
            field_length (int): If provided, limits the size of the region in
                which left/center/right alignment applies to the specified
                number of icon widths. Ignored if text_string contains "\\t".
        """
        field_length = min(field_length, pad_for_icon and 7 or 8)

        if '\t' in text_string:
            field_length = [pad_for_icon and 3 or 4] * 2
            text_string = [x.replace('\t', '') for x in text_string.split('\t', 1)]

            if isinstance(alignment, tuple):
                alignment = list(alignment)
            elif not isinstance(alignment, list):
                alignment = [alignment] * 2

            # Make room for the icon
            if pad_for_icon:
                field_length.insert(1,1)
                text_string.insert(1,'')
                alignment.insert(1, TextAlignment.LEFT)

            text_string = ''.join(self._text_conversion(*x) for x in zip(text_string, field_length, alignment)) + chr(0)
        else:
            text_string = self._text_conversion(text_string, field_length, alignment) + chr(0)
        text_length = len(text_string)

        if not pad_for_icon: # Cues the device to not leave space for the icon
            text_length += 256

        colour = min(colour, 32)
        line = max(1, min(line, 6))
        if isinstance(text_string, str):
            text_string = text_string.encode("ascii")
        self.devh.controlMsg(0x40, 24, text_string, text_length, (line - 1) * 256 + colour,
            self.usb_timeout_ms)


        # Return how long to wait before sending more data
        if isinstance(field_length, (list, tuple)):
            field_length = sum(field_length)
        field_length_as_percent = field_length * self.chars_per_icon / 22.0
        time.sleep(self.max_display_text_wait_ms * field_length_as_percent / 1000)

    def display_text_anywhere(self, pos_x, pos_y, text_string, colour):
        """Display text at an exact position on the device.

        Requires firmware >=1.04

        Args:
            pos_x (int): Number representing the position on the x-axis, in the
                range 0 to 320 minus the width of your icon, where 0 is the left
                edge of the display and 320 is the right edge.
            pos_y (int): Number representing the position on the y-axis, in the
                range 0 to 240 minus the width of your icon, where 0 is the top
                edge of the display and 240 is the bottom edge.
            text_string (str): The text to be displayed
            colour (int): The text colour from pylcdsysinfo.TextColours.
        """
        pos_x = max(0, min(pos_x, 320))
        pos_y = max(0, min(pos_y, 240))
        pos_y2 = pos_y + 40
        ba = struct.pack("<BBBBBBBB", pos_x >> 8, pos_x & 0xFF, pos_y >> 8, pos_y & 0xFF,
            319 >> 8, 319 & 0xFF, pos_y2 >> 8, pos_y2 & 0xFF)
        if isinstance(text_string, str):
            text_string = text_string.encode("ascii")
        ba += text_string

        text_length = len(text_string)
        colour = min(colour, 32)
        self.devh.controlMsg(0x40, 25, ba, text_length, colour, self.usb_timeout_ms)

        time.sleep(self.max_display_text_wait_ms / 1000)

    def dim_when_idle(self, value):
        """Set whether to dim the LCD backlight after the device has been idle for 10 seconds.

        Args:
            value (bool): If true, the LCD backlight will dim when the device is idle,
                otherwise the function will be disabled.
        """
        if value:
            self.devh.controlMsg(0x40, 17, "", 1, 0, self.usb_timeout_ms)
        else:
            self.devh.controlMsg(0x40, 17, "", 0, 266, self.usb_timeout_ms)

    def clear_lines(self, lines, colour):
        """Clear lines of the display using a coloured background.

        Args:
            lines (int): A number representing the lines to be cleared, using the
                values from pylcdsysinfo.TextLines OR'd together to form bits 0 to 5.
            colour (int): The background colour from pylcdsysinfo.BackgroundColours.
        """
        lines = max(1, min(lines, 63))
        self.devh.controlMsg(0x40, 26, "", lines, colour, self.usb_timeout_ms)
        time.sleep(self.clear_line_wait_ms * (count_bits_set(lines) / 6.0) * 0.8 / 1000)

    def display_cpu_info(self, cpu_util, cpu_temp, util_colour=TextColours.GREEN, temp_colour=TextColours.GREEN):
        """Display CPU utilisation and temperature information.

        Args:
            cpu_util (int): Percentage CPU utilisation, to a maximum of 4 digits,
                e.g. 994 will display 99.4%.
            cpu_temp (int): CPU temperature in degrees Celsius, to a maximum of 2 digits,
                e.g. 32 will display 32°C.
            util_colour (int): The colour of the CPU utilitisation, from
                pylcdsysinfo.BackgroundColours (defaults to GREEN).
            temp_colour (int): The colour of the CPU temperature, from
                pylcdsysinfo.BackgroundColours (defaults to GREEN).
        """
        self.devh.controlMsg(0x40, 21, chr(util_colour) + chr(temp_colour), cpu_util, cpu_temp, self.usb_timeout_ms)
        time.sleep(self.display_sysinfo_wait_ms / 1000.0)

    def display_ram_gpu_info(self, ram, gpu_temp, ram_colour=TextColours.GREEN, temp_colour=TextColours.GREEN):
        """Display available RAM and GPU temperature information.

        Args:
            ram (int): Available RAM in megabytes, to a maximum of 4 digits,
                e.g. 1994 will display 1994Mb.
            gpu_temp (int): GPU temperature in degrees Celsius, to a maximum of 2 digits,
                e.g. 32 will display 32°C.
            util_colour (int): The colour of the available RAM, from
                pylcdsysinfo.BackgroundColours (defaults to GREEN).
            temp_colour (int): The colour of the GPU temperature, from
                pylcdsysinfo.BackgroundColours (defaults to GREEN).
        """
        self.devh.controlMsg(0x40, 22, chr(ram_colour) + chr(temp_colour), ram, gpu_temp, self.usb_timeout_ms)
        time.sleep(self.display_sysinfo_wait_ms / 1000.0)

    def display_network_info(self, recv, sent, recv_colour=TextColours.GREEN, sent_colour=TextColours.GREEN, recv_mb=False, sent_mb=False):
        """Display network utilisation information.

        Args:
            recv (int): Current network receive rate, to a maximum of 4 digits,
                e.g. 1994 will display 1994Mb.
            sent (int): Current network transmit rate, to a maximum of 4 digits,
                e.g. 1994 will display 1994Mb.
            recv_colour (int): The colour of the network receive rate, from
                pylcdsysinfo.BackgroundColours (defaults to GREEN).
            sent_colour (int): The colour of the network transmit rate, from
                pylcdsysinfo.BackgroundColours (defaults to GREEN).
            recv_mb (bool): Display receive rate in kb instead of the default Mb.
            sent_mb (bool): Display transmit rate in kb instead of the default Mb.
        """
        self.devh.controlMsg(0x40, 20, chr(recv_mb) + chr(sent_mb) + chr(recv_colour) + chr(sent_colour), recv, sent, self.usb_timeout_ms)
        time.sleep(self.display_sysinfo_wait_ms / 1000.0)

    def display_fan_info(self, cpufan, chafan, cpufan_colour=TextColours.GREEN, chafan_colour=TextColours.GREEN):
        """Display fan speed information.

        Args:
            cpufan (int): Current CPU fan speed, to a maximum of 4 digits,
                e.g. 1994 will display 1994rpm.
            chafan (int): Current chassis fan speed, to a maximum of 4 digits,
                e.g. 1994 will display 1994rpm.
            cpufan_colour (int): The colour of the CPU fan speed, from
                pylcdsysinfo.BackgroundColours (defaults to GREEN).
            chafan_colour (int): The colour of the chassis fan speed, from
                pylcdsysinfo.BackgroundColours (defaults to GREEN).
        """
        self.devh.controlMsg(0x40, 23, chr(cpufan_colour) + chr(chafan_colour), cpufan, chafan, self.usb_timeout_ms)
        time.sleep(self.display_sysinfo_wait_ms / 1000.0)

    def send_command_to_flash(self, address, command):
        """Send command to SPI flash memory.

        Args:
            address (int): Address of sector or page to write
            command (int): 0=write enable, 1=write disable, 2=erase sector, 3=program page.
        """
        self.devh.controlMsg(0x40, 15, "", address, command, self.usb_timeout_ms)

    def write_rawimage_to_flash(self, sector, rawfile, check_sizes=True):
        """Write raw format bitmap image to SPI flash memory.

        Args:
            sector (int): Address of starting sector (0-511).
            bitmap (str/bytes/bytearray): Contents of raw bitmap image, in big endian 16bpp, RGB 5:6:5 format.
            check_sizes (bool): If True ensure image dimensions are either 36x36 or 320x240
        """
        rawfile = bytearray(rawfile)
        header = rawfile[0:8]
        # Sanity check
        be_ui2 = '>H'
        width = header[2:2 + 2]
        width = buffer(width)  # struct does not accept bytearray params
        width = struct.unpack(be_ui2, width)[0]
        height = header[4:4 + 2]
        height = buffer(height)
        height = struct.unpack(be_ui2, height)[0]
        
        if check_sizes:
            if (width != 36 or height != 36) and (width != 320 or height != 240):
                raise IOError("Image dimensions must be 36x36 or 320x240 (not %dx%d)" % (width, height))

        # TODO check rest of header?
        # expected_header = [16, 16, width1, width2, height1, height2, 1, 27]
        # TODO check length of rawfile?

        address = sector * 16

        # write enable flash
        self.send_command_to_flash(0, 5)

        file_index = 0
        # flash is 2Mb, consisting of 512 x 4096-byte sectors
        # each sector is 4096 bytes, consisting of 16 x 256-byte pages
        # each page is 256 bytes, consisting of 4 x 64-byte chunks
        for sector in range(0, 1 + int(len(rawfile) / 4096)):
            # erase sector
            self.send_command_to_flash(int(address / 16), 2)
            time.sleep(self.erase_sector_wait_ms / 1000)
            for page in range(0, 16):
                for chunk in range(0, 4):
                    temp_byte = bytearray(b"\x00" * 64)
                    for k in range(0, 64):
                        if file_index < len(rawfile):
                            temp_byte[k] = rawfile[file_index]
                        file_index += 1
                    # send 64 byte chunk to device's memory buffer, chunk=0,1,2,3
                    self.devh.controlMsg(0x40, 16, temp_byte, 0, chunk, self.usb_timeout_ms)

                # fetch 2-byte checksum calculated by device
                b = self.devh.controlMsg(0xc0, 12, 2, 0, 0, self.usb_timeout_ms)
                device_checksum = b[0] * 256 + b[1]

                # calculate checksum locally
                local_checksum = sum([
                    rawfile[i] for i in range(file_index - 256, min(file_index, len(rawfile)))
                ])

                if device_checksum != local_checksum:
                    raise IOError("Checksum error in page %4x (device=%d local=%d)" % (address, device_checksum, local_checksum))

                # write this 256-byte page to flash memory
                self.send_command_to_flash(address, 3)
                time.sleep(self.write_page_wait_ms / 1000)
                address += 1

        # write disable flash
        self.send_command_to_flash(0, 1)

    def write_image_to_flash(self, sector, bitmap, check_sizes=True):
        """Write bitmap image to SPI flash memory.

        Args:
            sector (int): Address of starting sector (0-511).
            bitmap (str): Contents of BMP bitmap image, in 16bpp, RGB 5:6:5 format.
        """
        rawfile = _bmp_to_raw(bitmap)
        self.write_rawimage_to_flash(sector, rawfile, check_sizes=check_sizes)

    def get_device_info(self):
        """Retrieve device information."""
        info = { }
        info['eeprom'] = self.devh.controlMsg(0xc0, 12, 8, 0, 1, self.usb_timeout_ms)
        info['serial'] = self.devh.controlMsg(0xc0, 12, 8, 0, 5, self.usb_timeout_ms)
        info['flash_id'] = self.devh.controlMsg(0xc0, 12, 2, 0, 6, self.usb_timeout_ms)
        info['flash_data'] = self.devh.controlMsg(0xc0, 12, 2, 0, 4, self.usb_timeout_ms)

        info['device_valid'] = (info['eeprom'][1] == 102 or info['eeprom'][1] == 103)
        info['picture_frame_mode'] = (info['eeprom'][4] == 136)
        info['8mb_flash'] = ((((info['eeprom'][6] / 2) & 1) == 0) and (((info['eeprom'][6] / 4) & 1) == 0))
        
        # this is a straight port of the C#, using buffer ba2
        flash_id = info['flash_id']
        x = flash_id[0], flash_id[1]
        if flash_id[0] == 0xEF and flash_id[1] == 0x14:
            flashcap = "2MB"
        elif flash_id[0] == 0xEF and flash_id[1] == 0x16:
            flashcap = "8MB"
        else:
            flashcap = "unknown capacity"
        info['flashcap'] = flashcap
        
        # this is a straight port of the C#, using buffer ba3
        # NOTE my firmware reports 1.03 although I believe my version is 1.05
        firmware_version = info['eeprom'][1]
        info['firmware_version'] = firmware_version / 100.0

        info['flash_data_version'] = info['flash_data'][0] / 100.0
        
        return info
