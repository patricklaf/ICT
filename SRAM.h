// SRAM
// � 2020 Patrick Lafarguette

#ifndef SRAM_H_
#define SRAM_H_

#include "Arduino.h"
#include "Bus.h"

// http://pcbjunkie.net/index.php/resources/ram-info-and-cross-reference-page/

enum {
	SRAM_CS,
	SRAM_WE,
	SRAM_GND,
	SRAM_VCC,
	SRAM_COUNT
};

typedef struct SRAM {
	uint8_t count;
	Bus address;
	Bus d;
	Bus q;
	int8_t signals[SRAM_COUNT];
} SRAM;

enum {
	SRAM_NONE,
	SRAM_2114,
};

unsigned int sram_identify(unsigned long code) {
	switch (code) {
	// 2114
	case 2114:
		return SRAM_2114;
	}
	return SRAM_NONE;
}

//////////
// 2114 //
//////////

// A0..A9
// 5, 6, 7, 4, 3, 2, 1, 17, 16, 15
uint8_t sram41xx_Ax[] = { 4, 5, 6, 3, 2, 1, 0, 16, 15, 14 };

Bus sram41xx_A = {
		sram41xx_Ax,
		10,
		0,
		0
};

// DQ1..DQ4
// 14 13 12 11
uint8_t sram41xx_DQx[] = { 13, 12, 11, 10 };

Bus sram41xx_DQ = {
		sram41xx_DQx,
		4,
		0,
		15
};

SRAM sram2114 = {
		18,
		sram41xx_A,
		sram41xx_DQ,
		sram41xx_DQ,
		// CS, WE, GND, VC
		// 8, 10, 9, 18
		{ 7, 9, 8, 17 }
};

SRAM sram;

#endif /* SRAM_H_ */
