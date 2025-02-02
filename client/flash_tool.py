import serial
import serial.tools.list_ports
import time
try:
  from sys import _MEIPASS
  PACKAGE_ROOT = _MEIPASS
except:
  PACKAGE_ROOT = None
from os import path
import subprocess

from tkinter import Tk
from tkinter import filedialog

class ProgrammingInterface:
  def __init__(self, port, baudrate = 1000000):
    self.serial = serial.Serial(port, baudrate, timeout = 1)

    # Give Arduino some time
    time.sleep(2)

  def getReadRequest(self, address, amount):
    return [
        0x05, 0x05,
        amount,
        (address >> 16) & 0xFF,
        (address >> 8) & 0xFF,
        address & 0xFF,
        0x00,
    ]

  def initialize(self):
    try:
      self.serial.write(b"\x01\x00")
      result = self.serial.read(1)
      assert result == b"\x81"
      done = True
    except:
      print("Error: Could not establish connection - try resetting your Arduino")
      exit(1)

    print("Connected to interface")
  
  def prettyHex(self, address, data):
    print(
      "0x{:04X}, Bytes: {:02}, Data: {}".format(
        address, len(data), data.hex()
      )
    )

  def read(self, address=0x00, size=0x10):
    # Write request and wait for response
    request = self.getReadRequest(address, size)
    self.serial.write(request)

    # Response has to be at least 2 bytes long, otherwise something went wrong
    while True:
      response = self.serial.read(size + 1)
      if len(response) > 1:
        break
    if len(response) > 1:
      status = response[0]
      body = response[1:]

      line = bytearray([size, (address >> 8) & 0xFF, address & 0xFF, 0x00]) + body
      crc = 0
      for nextbyte in line:
          crc = crc + nextbyte

      crc = (~crc + 1) & 0xFF
      line.append(crc)
      line = line.hex().upper()
      return self.prettyHex(address, body)

  def erase(self):
    self.serial.write(b"\x04\x00")
    assert self.serial.read(1) == b"\x84"
    print("Device erased")

  def reset(self):
    self.serial.write(b"\x02\x00")
    assert self.serial.read(1) == b"\x82"

  def write(self, file):
    lines = file.readlines()
    for line in lines:
      assert line[0] == ":"
      if line[7:9] != "00":
        continue

      length = int(line[1:3], 16)
      assert length + 4 < 256

      addressHi = int(line[3:5], 16)
      addressLo = int(line[5:7], 16)
      data = bytearray.fromhex(line[9 : 9 + length * 2])
      assert len(data) == length
      crc = addressHi + addressLo
      for i in range(len(data)):
        crc += data[i]
      crc = crc & 0xFF
      address = (addressLo + (addressHi << 8))
      expected = self.prettyHex(address, data)
      self.serial.write([0x3, len(data) + 5, len(data), 0, addressHi, addressLo, crc])
      self.serial.write(data)
      response = self.serial.read(1)
      if response != b"\x83":
        print("Error: Failed writing data")
        return None
      
      if not self.read(address, len(data)) == expected:
        print(f"response at address {address} did not match")
        return None

    self.reset()

  def deviceInfo(self):
    self.serial.write(b"\x08\x00")
    assert self.serial.read(1) == b"\x88"
    deviceId = self.serial.read(1)
    revision = self.serial.read(1)
    print("Device:   0x%s" % deviceId.hex())
    print("Revision: 0x%s" % revision.hex())
    return deviceId.hex()


def getPath(filename):
  if not PACKAGE_ROOT is None:
    return path.join(PACKAGE_ROOT, 'data', filename)
  else:
    return path.join(path.dirname(__file__), "..", "data", filename)

FLASHING_COMMAND_PREFIX = "-p ATmega328P -c arduino -P COM4 -U flash:w:"
FLASHING_COMMAND_SUFFIX = ":i"

class AppContext:
  def __init__(self):
    self._interface = None
    self._port = ""

  def createProgrammingTool(self):
    binary_path = getPath("avrdude.exe")
    try:
      command_args = [binary_path]
      flashing_command = FLASHING_COMMAND_PREFIX + getPath("firmware.hex") + FLASHING_COMMAND_SUFFIX
      command_args.extend(flashing_command.split())
      result = subprocess.run(command_args, check=True, capture_output=True, text=True)
    except subprocess.CalledProcessError as e:
      print(f"Command failed with exit code {e.returncode}")
      print(f"Error output: {e.stderr}")
  
  def choosePort(self): 
    print("choose port option:")
    ports = serial.tools.list_ports.comports()
    index = 0
    for port, desc, hwid in ports:
        print(f"Option {index}: Port: {port}, Description: {desc}, Hardware ID: {hwid}")
        index += 1
    validInputs = index-1
    selection = int(input(f"select an option[0-{validInputs}]: "))

    self._port = ports[selection][0]
  
  def getProgrammingInterface(self):
    '''
    get the programming interface. returns true if successful
    '''
    if self._interface is None:
      if self._port == "":
        print("no port initialized, can't create interface")
        return False
      self._interface = ProgrammingInterface(self._port)
      self._interface.initialize()
      if self._interface.deviceInfo() != "39":
        print("correct device not detected, perhaps something wrong with your connections")
        return False
    return True
  
  def flashFile(self):
    if not self.getProgrammingInterface():
      return
    print("pick file to flash")
    filePath = filedialog.askopenfilename(title="Select firmware file to flash: ", filetypes=[("Hex file", ('*.hex'))])
    
    print("Selected File:", filePath)

    try:
      file = open(filePath, "r")
    except:
      print("could not open selected file, exiting")
      return

    self._interface.write(file)


if __name__ == "__main__":
  root = Tk()

  app = AppContext()
  app.choosePort()
  shouldFlash = input("flash Arduino Uno to create programming tool? (y/n): ")
  if shouldFlash == 'y':
    app.createProgrammingTool()
  app.flashFile()