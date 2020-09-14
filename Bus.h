// Bus
// � 2020 Patrick Lafarguette

#ifndef BUS_H_
#define BUS_H_

#include "Arduino.h"

typedef struct Bus {
	uint8_t* pins;
	uint8_t count;
	uint16_t value;
	uint16_t high;
} Bus;

#if 0
// High-Z
// Pin mode INPUT, write 0
// DDR <- 0
// PORT <- 0

void HighZ(uint8_t pin) {
	pinMode(pin, INPUT);
	digitalWrite(pin, LOW);
}
#endif

#endif /* BUS_H_ */
