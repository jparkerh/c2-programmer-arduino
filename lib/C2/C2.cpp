/**
 * C2 implementation according to SiLabs Application Note 127:
 * https://www.silabs.com/documents/public/application-notes/AN127.pdf
 */

#include "C2.h"

C2::C2(volatile uint8_t *port, volatile uint8_t *ddr, volatile uint8_t *pin, uint8_t pinCk, uint8_t pinD, uint8_t pinLed) {
  _port = port;
  _ddr = ddr;
  _pin = pin;

  _pinCk = pinCk;
  _pinD = pinD;
  _pinLed = pinLed;

  device.id = 0x00;
  device.revision = 0x00;
}

void C2::setup() {
  Serial.begin(1000000);

  *_port = 0x00 | (1 << _pinCk);
  *_ddr = 0x00 | (1 << _pinCk);

  digitalWrite(_pinLed, LOW);
}

/**
 * Programming interface initialization sequence
 *
 * Page 15
 */
void C2::init() {
  // Force CK LOW for 2us
  *_port &= ~(1 << _pinCk);
  delayMicroseconds(25);

  //Force CK HIGH
  *_port |= (1 << _pinCk);

  // Wait at least 2us
  delayMicroseconds(3);

  // Enable programming
  addressWrite(FPCTL);

  dataWrite(0x02);
  delayMicroseconds(2);

  dataWrite(0x04);
  delayMicroseconds(2);

  dataWrite(0x01);

  // Wait at least 20ms
  delayMicroseconds(30);
}

void C2::deviceInfo() {
  addressWrite(C2Addresses::DEVICEID);
  device.id = dataRead();

  addressWrite(C2Addresses::REVID);
  device.revision = dataRead();
}

void C2::writeSfr(uint8_t address, uint8_t data) {
  addressWrite(address);
  dataWrite(data);
}

void C2::pulseClock() {
  // Force CK LOW for 2us
  *_port &= ~(1 << _pinCk);
  delayMicroseconds(2);

  //Force CK HIGH
  *_port |= (1 << _pinCk);

  // Wait at least 2us
  delayMicroseconds(2);
}

uint8_t C2::dataRead() {
  // Start bit
  pulseClock();

  noInterrupts();

  // set to output after start bit
  *_ddr |= (1 << _pinD);
  *_port &= ~(1 << _pinD);
  for(uint8_t i = 0; i < 4; i += 1) {
    // zeros for the data read instruction
    pulseClock();
  }
  // set to input and enable pull up
  *_ddr &= ~(1 << _pinD);

  uint8_t wait = 0;
  while (wait == 0) {
    pulseClock();
    wait = (*_pin & (1 << _pinD));
  }

  uint8_t output = 0;
  for(uint8_t i = 0; i < 8; i += 1) {
    pulseClock();
    // set the output bit if needed
    if (*_pin & (1 << _pinD)) {
      output |= (0x1 << i);
    }
  }

  pulseClock();

  interrupts();

  return output;
}

uint8_t C2::addressRead() {
  uint8_t addressInstruction = 0x2;

  // Start bit
  pulseClock();

  noInterrupts();

  // set to output after start bit
  *_ddr |= (1 << _pinD);
  for(uint8_t i = 0; i < 2; i += 1) {
    if((addressInstruction >> i) & 0x1) {
      *_port |= (1 << _pinD);
    } else {
      *_port &= ~(1 << _pinD);
    }
    pulseClock();
  }
  // set to input and enable pull up
  *_ddr &= ~(1 << _pinD);

  uint8_t output = 0;
  for(uint8_t i = 0; i < 8; i += 1) {
    pulseClock();
    // set the output bit if needed
    if (*_pin & (1 << _pinD)) {
      output |= (0x1 << i);
    }
  }
  pulseClock();

  return output;
}

void C2::dataWrite(uint8_t data) {
  uint16_t dataInstruction = data << 4;
  dataInstruction |= 0x1;

  // Start bit
  pulseClock();

  noInterrupts();

  // set to output after start bit
  *_ddr |= (1 << _pinD);
  for(uint8_t i = 0; i < 12; i += 1) {
    if((dataInstruction >> i) & 0x1) {
      *_port |= (1 << _pinD);
    } else {
      *_port &= ~(1 << _pinD);
    }
    
    pulseClock();
  }
  // set to input and enable pull up
  *_ddr &= ~(1 << _pinD);

  uint8_t wait = 0;
  while (wait == 0) {
    pulseClock();
    wait = (*_pin & (1 << _pinD));
  }

  pulseClock();

  interrupts();
}

void C2::addressWrite(uint8_t address) {
  uint16_t addressInstruction = address << 2;
  addressInstruction |= 0x3;

  // Start bit
  pulseClock();

  noInterrupts();

  // set to output after start bit
  *_ddr |= (1 << _pinD);

  for(uint8_t i = 0; i < 10; i += 1) {
    if((addressInstruction >> i) & 0x1) {
      *_port |= (1 << _pinD);
    } else {
      *_port &= ~(1 << _pinD);
    }

    pulseClock();
  }
  // set to input and enable pull up
  *_ddr &= ~(1 << _pinD);
  // *_port |= 1 << _pinD;

  pulseClock();

  interrupts();
}

uint8_t C2::pollBitHigh(uint8_t mask) {
  uint8_t retval;

  do {
    retval = addressRead();
  } while ((retval & mask) == 0);

  return retval;
}

uint8_t C2::pollBitLow(uint8_t mask) {
  uint8_t retval;

  do {
    retval = addressRead();
  } while (retval & mask);

  return retval;
}

uint8_t C2::readFlashBlock(uint16_t address, uint8_t *data, uint8_t bytes) {
  addressWrite(C2Addresses::FPDAT);
  dataWrite(BLOCK_READ);

  pollBitLow(_inBusy);
  pollBitHigh(_outReady);

  uint8_t value = dataRead();
  if(value != 0x0D) {
    return EXIT_FAILURE;
  }

  // Write high byte of address
  dataWrite(address >> 8);
  pollBitLow(_inBusy);

  // Write low byte of address
  dataWrite(address & 0xFF);
  pollBitLow(_inBusy);

  dataWrite(bytes);
  pollBitLow(_inBusy);
  pollBitHigh(_outReady);
  dataRead();

  for(uint8_t i = 0; i < bytes; i += 1) {
    pollBitHigh(_outReady);
    data[i] = dataRead();
  }

  return EXIT_SUCCESS;
}

uint8_t C2::writeFlashBlock(uint16_t address, uint8_t *data, uint8_t length) {
  addressWrite(FPDAT);
  dataWrite(BLOCK_WRITE);

  pollBitLow(_inBusy);
  pollBitHigh(_outReady);

  uint8_t value = dataRead();
  if(value != 0x0D) {
    return EXIT_FAILURE;
  }

  // Write high byte of address
  dataWrite(address >> 8);
  pollBitLow(_inBusy);

  // Write low byte of address
  dataWrite(address & 0xFF);
  pollBitLow(_inBusy);

  dataWrite(length);
  pollBitLow(_inBusy);

  for(uint8_t i = 0; i < length; i += 1) {
    dataWrite(data[i]);
    pollBitLow(_inBusy);
  }
  pollBitHigh(_outReady);

  value = dataRead();
  if(value != 0x0D) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

uint8_t C2::eraseDevice() {
  addressWrite(FPDAT);
  dataWrite(DEVICE_ERASE);

  pollBitLow(_inBusy);
  pollBitHigh(_outReady);

  uint8_t value = dataRead();
  if(value != 0x0D) {
    return EXIT_FAILURE;
  }

  dataWrite(0xDE);
  pollBitLow(_inBusy);

  dataWrite(0xAD);
  pollBitLow(_inBusy);

  dataWrite(0xA5);
  pollBitLow(_inBusy);
  pollBitHigh(_outReady);

  value = dataRead();
  if(value != 0x0D) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

uint8_t C2::getState() {
  return _state;
}

void C2::resetState() {
  _state = 0;
}

uint8_t C2::updateState(uint8_t data) {
  switch(_state) {
    case 0x00: {
      _messagePtr = 0;
      _message[_messagePtr++] = data;

      _state = 1;
    } break;

    case 0x01: {
      _bytesLeft = data;
      _message[_messagePtr++] = data;

      if(_bytesLeft == 0) {
        _state = 3;

        break;
      }

      _state = 2;
    } break;

    case 0x02: {
      _message[_messagePtr++] = data;
      _bytesLeft--;

      if(_bytesLeft == 0) {
        _state = 3;
      }
    } break;
  }

  return _state;
}

volatile uint8_t *C2::getMessage() {
  return _message;
}

void C2::loop() {
  unsigned char crc;
  unsigned char newcrc;
  unsigned long address;

  if(Serial.available()) {
    uint8_t data = Serial.read();
    updateState(data);

    if(_state == 3) {
      switch(_message[0]) {
        case Actions::ACK: {
          Serial.write(0x80);
        } break;

        case Actions::INIT: {
          init();
          deviceInfo();

          // switch(device.id) {
          //   case C2Devices::EFM8BB1:
          //   case C2Devices::EFM8BB2: {
          //     writeSfr(0xFF, 0x80);
          //     delayMicroseconds(5);
          //     writeSfr(0xEF, 0x02);
          //   } break;
          // }

          Serial.write(0x81);
          digitalWrite(_pinLed, HIGH);

          resetState();
        } break;

        case Actions::RESET: {
          // reset();
          resetState();

          Serial.write(0x82);
          digitalWrite(_pinLed, LOW);
        } break;

        case Actions::WRITE: {
          address = (((unsigned long)(_message[4]))<<8) + (((unsigned long)(_message[5]))<<0);
          crc = _message[6];
          newcrc = _message[5] + _message[4];
          for(uint8_t i = 0; i < _message[2]; i+= 1) {
            _flashBuffer[i] = _message[i+7];
          }

          for(uint8_t i = 0; i < _message[2]; i += 1) {
            newcrc += _flashBuffer[i];
          }

          if(crc != newcrc) {
            Serial.write(0x43);
            break;
          }

          uint8_t ch = _message[2];
          writeFlashBlock(address, _flashBuffer, ch);
          Serial.write(0x83);

          resetState();
        } break;

        case Actions::ERASE: {
          eraseDevice();
          resetState();

          Serial.write(0x84);
        } break;

        case Actions::READ: {
          uint8_t byteCount = _message[2];
          unsigned long addressPart1 = ((unsigned long)(_message[3])) << 16;
          unsigned long addressPart2 = ((unsigned long)(_message[4])) << 8;
          unsigned long addressPart3 = ((unsigned long)(_message[5])) << 0;

          address = addressPart1 | addressPart2 | addressPart3;
          readFlashBlock(address, _flashBuffer, byteCount);
          resetState();

          Serial.write(0x85);
          for(uint8_t i = 0; i < byteCount; i += 1) {
            Serial.write(_flashBuffer[i]);
          }
        } break;

        /*
        case 0x06: {
          writeAddress(_message[3]);
          writeData(_message[4]);
          resetState();

          Serial.write(0x86);
        } break;

        case 0x07: {
          writeAddress(_message[3]);
          data = readData();
          resetState();

          Serial.write(data);
          Serial.write(0x87);
        } break;
        */

        case Actions::INFO: {
          resetState();

          Serial.write(0x88);
          Serial.write(device.id);
          Serial.write(device.revision);
        }
      }
    }
  }
}