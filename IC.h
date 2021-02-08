// IC
// © 2020-2021 Patrick Lafarguette

#ifndef IC_H_
#define IC_H_

#include <Arduino.h>

#define ICT_VERSION "2.2.0"
#define ICT_DATE "08/02/2021"

#define ZIF_COUNT 40

// Package pins
const uint8_t ZIF[ZIF_COUNT] = {
		23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, A11, A10, A9, A8,
		A12, A13, A14, A15, 52, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32, 30, 28, 26, 24, 22
};

enum {
	TYPE_NONE,
	TYPE_LOGIC,
	TYPE_DRAM,
	TYPE_SRAM,
	TYPE_ROM,
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
