#include <util/atomic.h>
#include <TimerOne.h>
#include "manchester.h"
#include "buffer.h"

#define DEBUG
//#define DEBUG_ANALOG

#define SYMBOL_PERIOD 3000 // in microseconds
#define SAMPLE_PER_SYMBOL 4 // sample count per each symbol
#define THRESHOLD 40
// 3000 - 20
// 5000 - 40 - 15cm

#define WORD_SIZE 10 // start bit + 8 bit data + end bit

#define SYN 0xD5
#define STX 0x02
#define ETX 0x03

const int sensorPin = 3;

// START: receiver
int lastSensorValue = 0, lastSlope = 0;
int steadyCounter = 0;
unsigned long curWord = 0;
int symbolCounter = 0;
Buffer symbolBuf;

void receiveSymbol() { // interrupt
  int sensorValue = analogRead(sensorPin);
  int slope = 0;

#ifdef DEBUG_ANALOG
  Serial.println(sensorValue);
#endif

  if (sensorValue - lastSensorValue > THRESHOLD) { // FALLING
    slope = -1;
  } else if (lastSensorValue - sensorValue > THRESHOLD) { // RISING
    slope = 1;
  } else {
    slope = 0;
  }

  if (slope == 0 || slope == lastSlope || (slope != lastSlope && steadyCounter < SAMPLE_PER_SYMBOL - 2)) {
    if (steadyCounter < 4 * SAMPLE_PER_SYMBOL) {
      steadyCounter++;
    }
  } else {
    if (steadyCounter >= SAMPLE_PER_SYMBOL + 2) { // double
      curWord = (curWord << 1) | curWord & 0x1;
      symbolCounter++;

      if (symbolCounter >= WORD_SIZE * 2) {
        char ch;
        if (decodeWord(curWord, &ch)) {
          symbolBuf.push(ch);
          symbolCounter = 0;
        }
      }
    }

    if (slope == -1) {
      curWord = (curWord << 1) | 0x0;
    } else {
      curWord = (curWord << 1) | 0x1;
    }
    symbolCounter++;

    if (symbolCounter >= WORD_SIZE * 2) {
      char ch;
      if (decodeWord(curWord, &ch)) {
        symbolBuf.push(ch);
        symbolCounter = 0;
      }
    }

#ifndef DEBUG_ANALOG
    /*
        for (unsigned long mask = 0x80000; mask; mask >>= 1) {
          if (curWord & mask) {
            Serial.print('1');
          } else {
            Serial.print('0');
          }
        }
        Serial.print(' ');
        Serial.println(symbolCounter);
    */
#endif
    steadyCounter = 0;
  }

  lastSensorValue = sensorValue;
}
// END: receiver

void setup() {
  Serial.begin(9600);

  Timer1.initialize();
  Timer1.attachInterrupt(receiveSymbol, SYMBOL_PERIOD / SAMPLE_PER_SYMBOL);

#ifdef DEBUG
#ifndef DEBUG_ANALOG
  Serial.println("Receiver started!");
#endif
#endif
}

enum {
  Idle,
  Synchronized,
  Started,
};

int state = Idle;

void loop() {
  char ch;

  if (symbolBuf.pop(&ch)) {
    switch (state) {
      case Idle:
        if ((unsigned char)ch == SYN) {
          state = Synchronized;
          Serial.print("* ");
        }
        break;
      case Synchronized:
        if ((unsigned char)ch == STX) {
          state = Started;
          Serial.print("-> ");
        } else {
          state = Idle;
        }
        break;
      case Started:
        if ((unsigned char)ch == ETX) {
          state = Idle;
          Serial.println();
        } else {
          Serial.print(ch);
        }
        break;
    }
  } else {
    delay(1);
  }
}
