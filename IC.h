// IC
// � 2020 Patrick Lafarguette

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

#if FIO
typedef struct Pin {
	uint8_t bit;
	volatile uint8_t* mode; // DDR register
	volatile uint8_t* output; // Output register
	volatile uint8_t* input; // Input register
} Pin;
#endif

typedef struct Package {
	uint8_t count;
#if FIO
	Pin pins[ZIF_COUNT];
#else
	unsigned int pins[ZIF_COUNT];
#endif
} Package;

#if FIO
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
#endif

#endif /* IC_H_ */

//	section                      size      addr
//	.data                         810   8389120
//	.text                       49626         0
//	.bss                         1486   8389930
//	.comment                       17         0
//	.note.gnu.avr.deviceinfo       64         0
//	.debug_aranges                464         0
//	.debug_info                105847         0
//	.debug_abbrev                6729         0
//	.debug_line                 27435         0
//	.debug_frame                 6876         0
//	.debug_str                  12810         0
//	.debug_loc                  79214         0
//	.debug_ranges                8736         0
//	Total                      300114

// FIO and DIO2
//	section                      size      addr
//	.data                         790   8389120
//	.text                       47442         0
//	.bss                         1585   8389910
//	.comment                       17         0
//	.note.gnu.avr.deviceinfo       64         0
//	.debug_aranges                464         0
//	.debug_info                101094         0
//	.debug_abbrev                6719         0
//	.debug_line                 25088         0
//	.debug_frame                 7016         0
//	.debug_str                  12900         0
//	.debug_loc                  70137         0
//	.debug_ranges                5304         0
//	Total                      278620

// FIO not DIO2
//	section                      size      addr
//	.data                         790   8389120
//	.text                       47184         0
//	.bss                         1585   8389910
//	.comment                       17         0
//	.note.gnu.avr.deviceinfo       64         0
//	.debug_aranges                464         0
//	.debug_info                 99277         0
//	.debug_abbrev                6699         0
//	.debug_line                 24856         0
//	.debug_frame                 7016         0
//	.debug_str                  12472         0
//	.debug_loc                  69304         0
//	.debug_ranges                5112         0
//	Total                      274840
