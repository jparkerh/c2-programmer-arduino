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

## Running the released Binary

When using an .exe from the releases section of this repository, follow ths instructions below:
1. when first launching the tool you'll be prompted to see which port you'd like to use. Select the option for your arduino.
1. you'll then be asked whether you want to flash an Arduino Uno as your programming tool. Select y and enter to program the Arduino (only necessary once).
1. you'll then get a file picker dialog. Select the flash (.hex) file you'd like to program.

After this, you'll see the stream of binary data confirmed back from the device, and if no error messages are thrown it was successful!

## Running from source

1. ensure that you have both python and VSCode installed on your machine
1. install the platformio IDE extension in VSCode
1. build and upload the correct firmware for the Arduino you have. If you have a mega, use the "mega" environment in PlatformIO IDE.

### Installing python EFM8 client

1. install pyserial: ``pip install pyserial``
1. change directory to ./client

```
# this will start the same script as the executable above, follow those instructions
# do NOT select to flash the arduino, as you've already done this with PlatformIO
python flash_tool.py
```

## Compatibility
This should work on any Arduino Uno or clone running at 16MHz, or Arduino Mega (change the build def you build for in PlatformIO).

## Resources
* [SiLabs C2 Application note](https://www.silabs.com/documents/public/application-notes/AN127.pdf)

## License

See the license file. This is heavily based on the repository linked in source from https://github.com/stylesuxx
