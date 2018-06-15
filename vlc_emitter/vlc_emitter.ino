#include <util/atomic.h>
#include <TimerOne.h>
#include "manchester.h"
#include "buffer.h"

//#define DEBUG

#define SYMBOL_PERIOD 3000 // in microseconds

#define WORD_SIZE 10 // start bit + 8 bit data + end bit

#define SYN 0xD5
#define STX 0x02
#define ETX 0x03

// START: emitter
Buffer symbolBuf;
unsigned long symbols = 0; // only lower 20 bits are used
int symbolCounter = 0;

void debugPrintWord(unsigned long symbs) {
#ifdef DEBUG
  for (unsigned long mask = 0x80000; mask; mask >>= 1) {
    if (symbs & mask) {
      Serial.print('1');
    } else {
      Serial.print('0');
    }
  }
  Serial.println();
#endif
}

void emitSymbol() { // interrupt
  if (symbols == 0 || symbolCounter == 20) {
    char ch;

    if (!symbolBuf.pop(&ch)) { // buffer is empty
      symbols = 0x55555; // keep blinking
    } else {
#ifdef DEBUG
      Serial.print("emit character: ");
      Serial.println(ch);
#endif
      symbols = encodeWord(ch);
#ifdef DEBUG
      Serial.print("encoded word: ");
      debugPrintWord(symbols);
#endif
    }

    symbolCounter = 0;
  }

  if ((symbols & 0x80000) == 0x80000) {
    ledSet();
  } else {
    ledClear();
  }
  symbolCounter++;
  symbols <<= 1;
}
// END: emitter

// START: LED
const int led = 2;

inline void ledSet() {
  digitalWrite(led, HIGH);
}

inline void ledClear() {
  digitalWrite(led, LOW);
}
// END: LED

// START: frame
void writeChar(char ch) {
  while (!symbolBuf.push(ch)) {
    delay(1);
  }
}

void writeMargin(int margin) {
  for (int i = 0; i < margin; i++) {
    writeChar(0);
  }
}

void writeFrame(char *msg, int len, int margin = 2) {
  writeMargin(margin);

  writeChar(SYN);
  writeChar(STX);
  for (int i = 0; i < len; i++) {
    writeChar(*msg);
    msg++;
  }
  writeChar(ETX);
}
// END: frame

void setup() {
  pinMode(led, OUTPUT);
  Serial.begin(9600);

  Timer1.initialize();
  Timer1.attachInterrupt(emitSymbol, SYMBOL_PERIOD);

#ifdef DEBUG
  Serial.println("Emitter started!");
#endif
}

Buffer serialBuf;

void loop() {
#ifdef TRANSMIT_SERIAL
  if (Serial.available()) {
    char ch = Serial.read();

    while (!serialBuf.push(ch)) {
      delay(1);
    }

    Serial.println(ch);

    if (ch == '\r' || ch == '\n') {
      char buf[BUFFER_SIZE + 1], *p = buf;

      while (serialBuf.pop(p)) {
        p++;
      }
      *p = 0;

      writeFrame(buf, p - buf);
    }
  }
#else
  writeFrame("Hello!", 6, 1);
#endif
}
