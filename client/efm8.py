from hashlib import new
import serial
import serial.tools.list_ports
import time
import sys
import argparse
from tokenize import String

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
      sys.exit(1)

    print("Connected to interface")

  def read(self, file, start=0x00, size=0x3FFF, chunksize=0x10):
    for address in range(start, start + size, chunksize):
      # Write request and wait for response
      request = self.getReadRequest(address, chunksize)
      self.serial.write(request)

      # Response has to be at least 2 bytes long, otherwise something went wrong
      while True:
        response = self.serial.read(chunksize + 1)
        if len(response) > 1:
          break
      if len(response) > 1:
        status = response[0]
        body = response[1:]

        print("===============================================")
        print("address: %s" % hex(address))
        print("request: %s" % bytes(request).hex())
        print("response code: %s" % hex(status))
        print("response body: %s" % body.hex())

        line = bytearray([chunksize, (address >> 8) & 0xFF, address & 0xFF, 0x00]) + body
        crc = 0
        for nextbyte in line:
            crc = crc + nextbyte

        crc = (~crc + 1) & 0xFF
        line.append(crc)
        line = line.hex().upper()
        file.write(":%s\n" % line)

      else:
        break

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
      print(
        "0x{:04X}, Bytes: {:02X}, Data: {}".format(
          (addressLo + (addressHi << 8)), len(data), data.hex()
        )
      )
      self.serial.write([0x3, len(data) + 5, len(data), 0, addressHi, addressLo, crc])
      self.serial.write(data)
      response = self.serial.read(1)
      if response != b"\x83":
          print("Error: Failed writing data")
          return None

    self.reset()

  def deviceInfo(self):
    self.serial.write(b"\x08\x00")
    assert self.serial.read(1) == b"\x88"
    deviceId = self.serial.read(1)
    revision = self.serial.read(1)
    print("Device:   0x%s" % deviceId.hex())
    print("Revision: 0x%s" % revision.hex())

parser = argparse.ArgumentParser(description='Interact with the Arduino based EFM8 C2 interface')
parser.add_argument('--action', metavar='ACTION', type=str, default="info",
                    help='Action to perform: read, write, info',
                    choices=['read', 'write', 'info'])
parser.add_argument('--port', metavar='PORT', type=str, default=None,
                    help='Port to use')
parser.add_argument('--file', metavar='FILE', type=str, default=None,
                    help='File to write to or read from')

args = parser.parse_args()

if args.port is None:
  print("must select a port, choose one of the below paths:")
  ports = serial.tools.list_ports.comports()
  for port, desc, hwid in ports:
      print(f"Port: {port}, Description: {desc}, Hardware ID: {hwid}")
  exit()

if args.action != "info" and args.file is None:
  print("file must be defined to read or write flash")
  exit()

interface = ProgrammingInterface(args.port)
interface.initialize()
interface.deviceInfo()

if args.action == 'read':
  if not args.file:
    parser.print_usage()
    parser.exit()

  file = open(args.file, "w")

  # Fetch the flash segment
  interface.read(file, 0x0, 0x1840)

  # Reading the bootloader on BB51 does not seem to bepossible since we are not
  # getting a response from this address space
  # TODO: Fetch the bootloader on BB51
  # if args.mcu == 'BB51':
  #  interface.read(file, 0xF000, 0x0800)

  file.write(":00000001FF\n")

if args.action == 'erase':
  interface.erase()

if args.action == 'write':
  if not args.file:
    parser.print_usage()
    parser.exit()

  file = open(args.file, "r")

  interface.erase()
  interface.write(file)