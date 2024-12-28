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

### Installing python EFM8 client
change directory to ./client, then use pipenv to create an virtual env, install dependencies and enter the env:
```
pipenv install
pipenv shell
```

then run the client like so (assuming `/dev/ttyUSB0` is your Arduino):

```
python efm8.py read /dev/ttyUSB0 output.hex
python efm8.py write /dev/ttyUSB0 input.hex
python efm8.py erase /dev/ttyUSB0
```

## Compatibility
This should work on any Arduino Uno or clone running at 16MHz, or Arduino Mega (change the build def you build for in PIO).

## Resources
* [SiLabs C2 Application note](https://www.silabs.com/documents/public/application-notes/AN127.pdf)
