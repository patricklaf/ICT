// Bus
// © 2020 Patrick Lafarguette

#ifndef BUS_H_
#define BUS_H_

#include <Arduino.h>

typedef struct Bus {
	uint8_t *pins;
	uint8_t count;
	uint16_t value;
	uint16_t high;
} Bus;

#endif /* BUS_H_ */
