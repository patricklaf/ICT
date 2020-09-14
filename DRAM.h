// DRAM
// � 2020 Patrick Lafarguette

#ifndef DRAM_H_
#define DRAM_H_

#include "Arduino.h"
#include "Bus.h"

// http://pcbjunkie.net/index.php/resources/ram-info-and-cross-reference-page/

// https://www.c64-wiki.com/wiki/RAM

// http://www.minuszerodegrees.net/memory/4164.htm
// http://www.minuszerodegrees.net/memory/41256.htm

// 4164

//	MB8264-15 (Fairchild)
//	MB8264A-15 (Fujitsu)
//	HM4864, HYB4164P2BD (Hitachi)
//	M3764A-15 (OKI)
//	MT4264-25 (Micron)
//	M5K4164AP-15, M5K4164 ANP-10 (Mitsubishi)
//	MK4564N-20 (Mostek)
//	MCM4164BP15, MCM6665BP20, MCM6665AP (Motorola)
//	D4164C-2, D4164C-15, uPD 4164-1 (NEC)
//	KM4164B-15 (Samsung)
//	TMM4164 C-3, TMS4164-15NL, TMS4164-20NL (Toshiba)

// 41256

//	TMS4256 (Toshiba)
//	MCM6256 (Motorola)
//	HM51256 (Hitachi)
//	MB81256 (Fujitsu)
//	HYB41256 (Siemens)
//	KM41256 (Samsung)
//	MSM41256 (OKI)
//	uPD41256 (NEC)

enum {
	DRAM_RAS,
	DRAM_CAS,
	DRAM_WE, // /W
	DRAM_OE, // /G
	DRAM_GND, // VDD
	DRAM_VCC,
	DRAM_COUNT
};

typedef struct DRAM {
	uint8_t count;
	Bus row;
	Bus column;
	Bus d;
	Bus q;
	int8_t signals[DRAM_COUNT];
} DRAM;

enum {
	DRAM_NONE,
	DRAM_4164,
	DRAM_41256,
	DRAM_4416,
	DRAM_44256,
};

unsigned int dram_identify(unsigned long code) {
	switch (code) {
	// 4164
	case 3764:
	case 4164:
	case 4264:
	case 4564:
	case 4864:
	case 6665:
	case 8264:
		return DRAM_4164;
	// 41256
	case 4256:
	case 6256:
	case 41256:
	case 51256:
	case 81256:
		return DRAM_41256;
	// 4416
	case 4416:
		return DRAM_4416;
	// 44256
	case 44256:
		return DRAM_44256;
	}
	return DRAM_NONE;
}

/////////////////
// 4164, 41256 //
/////////////////

// A0..A8
// 5, 7, 6, 12, 11, 10, 13, 9, 1
uint8_t dram41xx_Ax[] = { 4, 6, 5, 11, 10, 9, 12, 8, 0 };

Bus dram41xx_A = {
		dram41xx_Ax,
		8,
		0,
		0
};

// D
// 2
uint8_t dram41xx_Dx[] = { 1 };

Bus dram41xx_D = {
		dram41xx_Dx,
		1,
		0,
		1
};

// Q
// 14
uint8_t dram41xx_Qx[] = { 13 };

Bus dram41xx_Q = {
		dram41xx_Qx,
		1,
		0,
		1
};

DRAM dram41xx = {
		16,
		dram41xx_A,
		dram41xx_A,
		dram41xx_D,
		dram41xx_Q,
		// RAS, CAS, WE, OE, GND, VC
		// 4, 15, 3, none, 16, 8
		{ 3, 14, 2, -1, 15, 7 }
};

//////////
// 4416 //
//////////

// A0..A7
// 14, 13, 12, 11, 8, 7, 6, 10
uint8_t dram4416_RAx[] = { 13, 12, 11, 10, 7, 6, 5, 9 };

Bus dram4416_RA = {
		dram4416_RAx,
		8,
		0,
		0
};

// A1..A6
// 13, 12, 11, 8, 7, 6
uint8_t dram4416_CAx[] = { 12, 11, 10, 7, 6, 5 };

Bus dram4416_CA = {
		dram4416_CAx,
		6,
		0,
		0
};

// DQ1..DQ4
// 2, 3, 15, 17
uint8_t dram4416_DQx[] = { 1, 2, 14, 16 };

Bus dram4416_DQ = {
		dram4416_DQx,
		4,
		0,
		15
};

DRAM dram4416 = {
		18,
		dram4416_RA,
		dram4416_CA,
		dram4416_DQ,
		dram4416_DQ,
		// RAS, CAS, WE, OE, GND, VC
		// 5, 16, 4, 1, 18, 9
		{ 4, 15, 3, 0, 17, 8 }
};

///////////
// 44256 //
///////////

// A0..A8
// 6, 7, 8, 9, 11, 12, 13, 14, 15
uint8_t dram44256_Ax[] = { 5, 6, 7, 8, 10, 11, 12, 13, 14 };

Bus dram44256_A = {
		dram44256_Ax,
		9,
		0,
		0
};

// DQ1..DQ4
// 1, 2, 18, 19
uint8_t dram44256_DQx[] = { 0, 1, 17, 18 };

Bus dram44256_DQ = {
		dram44256_DQx,
		4,
		0,
		15
};

DRAM dram44256 = {
		20,
		dram44256_A,
		dram44256_A,
		dram44256_DQ,
		dram44256_DQ,
		// RAS, CAS, WE, OE, GND, VC
		// 4, 17, 3, 16, 20, 10
		{ 3, 16, 2, 15, 19, 9 }
};

DRAM dram;

#endif /* DRAM_H_ */
