"""Reset the board over USB-Serial/JTAG and print the serial log for N seconds.
Usage (inside the pio container):  python tools/bootlog.py [/dev/ttyACM0] [seconds] [noreset]
"""
import sys
import time

import serial

port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
secs = float(sys.argv[2]) if len(sys.argv) > 2 else 8
reset = not (len(sys.argv) > 3 and sys.argv[3] == "noreset")

if reset:
    s = serial.Serial(port, 115200, timeout=0.1)
    # Same sequence esptool uses for a hard reset on the USB-Serial/JTAG peripheral.
    s.dtr = False
    s.rts = True
    time.sleep(0.1)
    s.rts = False
    s.close()

end = time.time() + secs
while time.time() < end:
    try:
        s = serial.Serial(port, 115200, timeout=0.2)   # the port re-enumerates after reset
        break
    except serial.SerialException:
        time.sleep(0.1)
else:
    sys.exit("port did not come back")

while time.time() < end:
    data = s.read(512)
    if data:
        sys.stdout.write(data.decode("utf-8", "replace"))
        sys.stdout.flush()
