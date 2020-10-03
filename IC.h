// IC
// © 2020 Patrick Lafarguette

#ifndef IC_H_
#define IC_H_

#include "Arduino.h"

#define ZIF_COUNT 20

// Package pins
const uint8_t ZIF[ZIF_COUNT] = { 30, 32, 34, 36, 38, 40, 42, 44, 50, 48, 49, 47, 45, 43, 41, 39, 37, 35, 33, 31 };

enum {
	TYPE_NONE,
	TYPE_LOGIC,
	TYPE_DRAM,
	TYPE_SRAM,
};

typedef struct IC {
	uint8_t type;
	uint8_t count;
	String code;
	String description;
} IC;

typedef struct Pin {
	uint8_t bit;
	volatile uint8_t* mode; // DDR register
	volatile uint8_t* output; // Output register
	volatile uint8_t* input; // Input register
} Pin;

typedef struct Package {
	uint8_t count;
	Pin pins[ZIF_COUNT];
} Package;

void ic_pin_mode(Pin& pin, uint8_t mode) {
	if (mode == INPUT) {
		*pin.mode &= ~pin.bit;
		*pin.output &= ~pin.bit;
	} else if (mode == INPUT_PULLUP) {
		*pin.mode &= ~pin.bit;
		*pin.output |= pin.bit;
	} else {
		// Output
		*pin.mode |= pin.bit;
	}
}

void ic_pin_write(Pin& pin, uint8_t value) {
	if (value == LOW) {
		*pin.output &= ~pin.bit;
	} else {
		*pin.output |= pin.bit;
	}
}

uint8_t ic_pin_read(Pin& pin) {
	return *pin.input & pin.bit ? HIGH : LOW;
}

#endif /* IC_H_ */
