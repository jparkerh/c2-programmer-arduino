#include "Arduino.h"
#include "C2.h"

/**
 * this source is all based on https://github.com/bird-sanctuary/arduino-bi-directional-dshot
 * which was based on: https://gist.github.com/racerxdl/c9a592808acdd9cd178e6e97c83f8baf
 * which was based on: https://github.com/jaromir-sukuba/efm8prog/
 */

#ifdef MEGA_CONFIG
  #define PORT PORTB
  #define DDR DDRB
  #define PIN PINB
  #define C2D_PIN 4
  #define C2CK_PIN 5
#else //MEGA_CONFIG
  #define PORT PORTD
  #define DDR DDRD
  #define PIN PIND
  #define C2D_PIN 2
  #define C2CK_PIN 3
#endif


C2 *c2 = new C2(&PORT, &DDR, &PIN, (uint8_t) C2CK_PIN, (uint8_t) C2D_PIN, (uint8_t) LED_BUILTIN);

void setup() {
  c2->setup();
}

void loop() {
  c2->loop();
}
