// TMS41xx
// © 2020 Patrick Lafarguette

#ifndef TMS41XX_H_
#define TMS41XX_H_

#include "Arduino.h"

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

typedef struct TMS41xx {
	unsigned int row;
	unsigned int column;
	unsigned int bits; // Numbers of address bits
	unsigned int high;
	unsigned int d;
	unsigned int q;
} TMS41xx;

enum {
	TMS4164 = 1,
	TMS41256
};

unsigned int tms41xx_identify(unsigned long code) {
	switch (code) {
	// 4164
	case 3764:
	case 4164:
	case 4264:
	case 4564:
	case 4864:
	case 6665:
	case 8264:
		return TMS4164;
	// 41256
	case 4256:
	case 6256:
	case 41256:
	case 51256:
	case 81256:
		return TMS41256;
	}
	return 0;
}

#endif /* TMS41XX_H_ */
