// Bus
// © 2020-2021 Patrick Lafarguette

#ifndef BUS_H_
#define BUS_H_

#include <Arduino.h>

typedef struct Bus {
	uint8_t pins[ZIF_COUNT];
	uint8_t width;
	uint8_t data;
	uint32_t address;
	uint32_t high;
} Bus;

#endif /* BUS_H_ */
