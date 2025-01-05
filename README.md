# EFM8 Flash Tool

## C2 Interface
This sketch includes a C2 interface. Connect up the pins as indicated below for an UNO and MEGA, respectively.

### Uno
```
D  Pin 2
CK Pin 3
```

### Mega
```
D  Pin 10
CK Pin 11
```

### Verify dependencies

1. ensure that you have both python and VSCode installed on your machine
1. install the platformio IDE extension in VSCode
1. build and upload the correct firmware for the Arduino you have. If you have a mega, use the "mega" environment in PlatformIO IDE.

### Installing python EFM8 client

1. change directory to ./client
1. install pyserial: ``pip install pyserial``
1. then run the client like so (assuming `COM2` is your Arduino):

```
# this will print out a list of ports present on your machine. select the one with the arduino.
python efm8.py

# this should query the device. Ensure that this returns 0x39 as the device ID.
python efm8.py --port COM2

# this will flash the hex file to the device.
python efm8.py --port COM2 --action write --file <provided hex file>
```

## Compatibility
This should work on any Arduino Uno or clone running at 16MHz, or Arduino Mega (change the build def you build for in PlatformIO).

## Resources
* [SiLabs C2 Application note](https://www.silabs.com/documents/public/application-notes/AN127.pdf)

## License

See the license file. This is heavily based on the repository linked in source from https://github.com/stylesuxx
