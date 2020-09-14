// IC Tester
// © 2020 Patrick Lafarguette
//
// 14/09/2020	1.3.0
//				Remove DIO2 library.
//				Add function ic_package_idle.
//
// 12/09/2020	DRAM support. 4416 family.
//				Improved performances with ic_pin_xx functions.
//
// 11/09/2020	Code optimization.
//				Improved performances.
//				DIO2 library.
//
// 10/09/2020	Code rework to accommodate more RAMs.
//				SRAM support. 2114 family.
//				DRAM support. 44256 family.
//
// 09/09/2020	Increase ZIF pins to 20.
//				RAM content is now correctly displayed.
//
// 07/09/2020	Dynamic creation of package.
//				RAM content is now correctly displayed.
//
// 04/09/2020	Test function for identified IC.
//
// 01/09/2020	1.2.0
// 				Redo function for logic test.
//				Screen capture for documentation.
//
// 31/08/2020 	1.1.0
//				UI optimization.
//				Code optimization.
//				DRAM support. 4164 and 41256 family.
//
// 13/08/2020	1.0.0
//				Initial version.

#include <MCUFRIEND_kbv.h>
#include <SdFat.h>
#include <TouchScreen.h>

#define FIO 1
#define DIO2 0

#if DIO2
#include <DIO2.h>

#define PIN_MODE(...) pinMode2(__VA_ARGS__)
#define DIGITAL_READ(...) digitalRead2(__VA_ARGS__)
#define DIGITAL_WRITE(...) digitalWrite2(__VA_ARGS__)
#else
#define PIN_MODE(...) pinMode(__VA_ARGS__)
#define DIGITAL_READ(...) digitalRead(__VA_ARGS__)
#define DIGITAL_WRITE(...) digitalWrite(__VA_ARGS__)
#endif

#include "IC.h"
#include "DRAM.h"
#include "SRAM.h"
#include "Stack.h"

// Set one of the language to 1.
// French UI, currently not viable as screen only support
// unaccented US-ASCII character set.

#define ENGLISH 1
#define FRENCH 0

#include "Language.h"

// TFT calibration
// See TouchScreen_Calibr_native example in MCUFRIEND_kbv library
const int XP = 8, XM = A2, YP = A3, YM = 9;
const int TS_LEFT = 908, TS_RT = 122, TS_TOP = 85, TS_BOT = 905;

MCUFRIEND_kbv tft;
// XP (LCD_RS), XM (LCD_D0) resistance is 345 Ω
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 345);

// SD
SdFatSoftSpi<12, 11, 13> fat;

Stack<IC*> ics;
Stack<String*> lines;

#define TEST 0 // 1 to enable test cases, 0 to disable
#define TIME 1 // 1 to enable time, 0 to disable
#define DEBUG 1 // 1 to enable serial debug messages, 0 to disable
#define CAPTURE 0 // 1 to enable screen captures,  0 to disable

#if DEBUG
#define Debug(...) Serial.print(__VA_ARGS__)
#define Debugln(...) Serial.println(__VA_ARGS__)
#else
#define x(...)
#define Debug(...)
#define Debugln(...)
#endif

typedef enum states {
	state_startup,
	state_media,
	state_menu,
	state_identify_logic,
	state_test_logic,
	state_test_ram,
	state_package_dip14, // Only logic
	state_package_dip16, // Only logic
	state_keyboard,
	state_identified,
	state_tested,
} states_t;

typedef enum actions {
	action_idle,
	action_identify_logic,
	action_test_logic,
	action_test_ram,
} actions_t;

typedef enum buttons {
	button_identify_logic,
	button_test_logic,
	button_test_ram,
	// Identify logic
	button_dip14 = 0,
	button_dip16,
	// Identified
	button_test = 0,
	button_previous,
	button_next,
	button_escape,
	// Test
	button_redo = 0,
	// Keyboard
	button_zero = 0,
	button_one,
	button_two,
	button_three,
	button_four,
	button_five,
	button_six,
	button_seven,
	button_eight,
	button_nine,
	button_enter,
	button_clear,
	button_del,
	button_count,
} buttons_t;

typedef struct Worker {
	uint8_t state;
	uint8_t action;
	// TFT
	int x;
	int y;
	// IC
	Package package;
	String code;
	// Identify
	unsigned int index;
	// Test
	bool found;
	bool ok;
	unsigned int success;
	unsigned int failure;
	// UI
	uint16_t color;
	unsigned int indicator;
	unsigned int ram;
} Worker;

Worker worker;

// UI size
#define TFT_WIDTH 320
#define TFT_HEIGHT 240

// UI areas
#define AREA_HEADER 0
#define AREA_CONTENT 80
#define AREA_FOOTER 180

// UI buttons
#define BUTTON_X 33
#define BUTTON_Y 90
#define BUTTON_W 52
#define BUTTON_H 48
#define BUTTON_SPACING_X 12
#define BUTTON_SPACING_Y 12
#define BUTTON_TEXTSIZE 2

// UI input
#define INPUT_HEIGHT 54
#define TEXT_X 8
#define TEXT_Y 12
#define TEXT_SIZE 4
#define TEXT_LENGTH 7

// UI RAM
#define RAM_TOP 140
#define RAM_BOX 40
#define RAM_SIZE (RAM_BOX - 2 * 2)
#define RAM_HALF (RAM_SIZE / 2)
#define RAM_BOTTOM (RAM_TOP + RAM_SIZE)

// Color scheme
#define COLOR_BACKGROUND TFT_NAVY
#define COLOR_TEXT TFT_YELLOW

#define COLOR_OK TFT_GREEN
#define COLOR_KO TFT_ORANGE
#define COLOR_GOOD TFT_GREEN
#define COLOR_BAD TFT_RED
#define COLOR_MIXED TFT_ORANGE
#define COLOR_SKIP TFT_DARKGREY

#define COLOR_LABEL TFT_WHITE
#define COLOR_ENTER TFT_BLUE
#define COLOR_DELETE TFT_GREEN
#define COLOR_ESCAPE TFT_RED
#define COLOR_KEY TFT_BLACK
#define COLOR_CODE TFT_WHITE
#define COLOR_DESCRIPTION TFT_CYAN

Adafruit_GFX_Button buttons[button_count];

// Database filename
#define FILENAME "database.txt"

//#pragma GCC optimize ("-O2")
//
//#pragma GCC push_options

/////////
// Bus //
/////////
const uint16_t DATA[] = {
		(uint16_t)(1 << 0),
		(uint16_t)(1 << 1),
		(uint16_t)(1 << 2),
		(uint16_t)(1 << 3),
		(uint16_t)(1 << 4),
		(uint16_t)(1 << 5),
		(uint16_t)(1 << 6),
		(uint16_t)(1 << 7),
		(uint16_t)(1 << 8),
		(uint16_t)(1 << 9),
		(uint16_t)(1 << 10),
		(uint16_t)(1 << 11),
		(uint16_t)(1 << 12),
		(uint16_t)(1 << 13),
		(uint16_t)(1 << 14),
		(uint16_t)(1 << 15)
};

#if FIO
void ic_bus_write(Bus& bus) {
	for (uint8_t index = 0; index < bus.count; ++index) {
		ic_pin_write(worker.package.pins[bus.pins[index]], bus.value & DATA[index]);
	}
}

void ic_bus_read(Bus& bus) {
	bus.value = 0;
	for (uint8_t index = 0; index < bus.count; ++index) {
		if (ic_pin_read(worker.package.pins[bus.pins[index]])) {
			bus.value |= DATA[index];
		}
	}
}

void ic_bus_output(Bus& bus) {
	for (uint8_t index = 0; index < bus.count; ++index) {
		ic_pin_mode(worker.package.pins[bus.pins[index]], OUTPUT);
	}
}

void ic_bus_input(Bus& bus) {
	for (uint8_t index = 0; index < bus.count; ++index) {
		ic_pin_mode(worker.package.pins[bus.pins[index]], INPUT);
	}
}
#else
void ic_bus_write(Bus& bus) {
	for (uint8_t index = 0; index < bus.count; ++index) {
		DIGITAL_WRITE(worker.package.pins[bus.pins[index]], bus.value & DATA[index]);
	}
}

void ic_bus_read(Bus& bus) {
	bus.value = 0;
	for (uint8_t index = 0; index < bus.count; ++index) {
		if (DIGITAL_READ(worker.package.pins[bus.pins[index]])) {
			bus.value |= DATA[index];
		}
	}
}

void ic_bus_output(Bus& bus) {
	for (uint8_t index = 0; index < bus.count; ++index) {
		PIN_MODE(worker.package.pins[bus.pins[index]], OUTPUT);
	}
}

void ic_bus_input(Bus& bus) {
	for (uint8_t index = 0; index < bus.count; ++index) {
		ic_pin_mode(worker.package.pins[bus.pins[index]], INPUT);
		PIN_MODE(worker.package.pins[bus.pins[index]], INPUT);
	}
}
#endif

void ic_bus_data(Bus& bus, uint8_t bit, const bool alternate) {
	uint8_t mask = alternate ? HIGH : LOW;
	bus.value = 0;
	for (uint8_t index = 0; index < bus.count; ++index) {
		if (bit) {
			bus.value |= DATA[index];
		}
		bit ^= mask;
	}
	Debug("data 0b");
	Debugln(bus.value, BIN);
}

//////////
// DRAM //
//////////

bool ic_dram_oe(DRAM &dram) {
	return dram.signals[DRAM_OE] > 0;
}

#if FIO
void ic_dram_write(DRAM& dram) {
	// Row
	ic_bus_write(dram.row);
	ic_pin_write(worker.package.pins[dram.signals[DRAM_RAS]], LOW);
	// WE
	ic_pin_write(worker.package.pins[dram.signals[DRAM_WE]], LOW);
	// D
	ic_bus_write(dram.d);
	// Column
	ic_bus_write(dram.column);
	ic_pin_write(worker.package.pins[dram.signals[DRAM_CAS]], LOW);
	// Idle
	ic_pin_write(worker.package.pins[dram.signals[DRAM_WE]], HIGH);
	ic_pin_write(worker.package.pins[dram.signals[DRAM_RAS]], HIGH);
	ic_pin_write(worker.package.pins[dram.signals[DRAM_CAS]], HIGH);
}

void ic_dram_read(DRAM& dram) {
	// Row
	ic_bus_write(dram.row);
	ic_pin_write(worker.package.pins[dram.signals[DRAM_RAS]], LOW);
	// Column
	ic_bus_write(dram.column);
	ic_pin_write(worker.package.pins[dram.signals[DRAM_CAS]], LOW);
	// OE
	if (ic_dram_oe(dram)) {
		ic_pin_write(worker.package.pins[dram.signals[DRAM_OE]], LOW);
	}
	// Q
	ic_bus_read(dram.q);
	// Idle
	ic_pin_write(worker.package.pins[dram.signals[DRAM_RAS]], HIGH);
	ic_pin_write(worker.package.pins[dram.signals[DRAM_CAS]], HIGH);
	// OE
	if (ic_dram_oe(dram)) {
		ic_pin_write(worker.package.pins[dram.signals[DRAM_OE]], HIGH);
	}
}
#else
void ic_dram_write(DRAM& dram) {
	// Row
	ic_bus_write(dram.row);
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_RAS]], LOW);
	// WE
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_WE]], LOW);
	// D
	ic_bus_write(dram.d);
	// Column
	ic_bus_write(dram.column);
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_CAS]], LOW);
	// Idle
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_WE]], HIGH);
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_RAS]], HIGH);
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_CAS]], HIGH);
}

void ic_dram_read(DRAM& dram) {
	// Row
	ic_bus_write(dram.row);
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_RAS]], LOW);
	// Column
	ic_bus_write(dram.column);
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_CAS]], LOW);
	// OE
	if (ic_dram_oe(dram)) {
		DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_OE]], LOW);
	}
	// Q
	ic_bus_read(dram.q);
	// Idle
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_RAS]], HIGH);
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_CAS]], HIGH);
	// OE
	if (ic_dram_oe(dram)) {
		DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_OE]], HIGH);
	}
}
#endif

void ic_dram_fill(DRAM& dram, const bool alternate = false) {
	uint16_t mask = alternate ? dram.d.high : 0;
	worker.color = COLOR_GOOD;
	for (dram.column.value = 0; dram.column.value < dram.column.high; ++dram.column.value) {
		worker.success = 0;
		worker.failure = 0;
		ic_bus_output(dram.d);
		// Inner loop is RAS to keep refreshing rows while writing a full column
		for (dram.row.value = 0; dram.row.value < dram.row.high; ++dram.row.value) {
			ic_dram_write(dram);
			dram.d.value ^= mask;
		}
		ic_bus_input(dram.q);
		// Inner loop is RAS to keep refreshing rows while reading back a full column
		for (dram.row.value = 0; dram.row.value < dram.row.high; ++dram.row.value) {
			ic_dram_read(dram);
			if (dram.d.value != dram.q.value) {
				Debug("D ");
				Debug(dram.d.value, BIN);
				Debug(", Q ");
				Debugln(dram.q.value, BIN);
				worker.failure++;
				worker.color = COLOR_BAD;
			} else {
				worker.success++;
			}
			dram.d.value ^= mask;
		}
		ui_draw_indicator((worker.success ? (worker.failure ? COLOR_MIXED : COLOR_GOOD) : COLOR_BAD));
#if CAPTURE
		if (ts_touched()) {
			sd_screen_capture();
		}
#endif
	}
	ui_draw_ram();
}

#if FIO
void ic_dram(DRAM& dram) {
	// UI
	worker.ram = 0;
	worker.color = TFT_WHITE;
	for (unsigned int index = 0; index < 4; ++index) {
		ui_draw_ram();
	}
	worker.ram = 0;
#if TIME
	// Timer
	unsigned long start = millis();
#endif
	// Setup
	dram.row.high = 1 << dram.row.count;
	dram.column.high = 1 << dram.column.count;
	for (unsigned int index = 0; index < dram.q.count; ++index) {
		ic_pin_mode(worker.package.pins[dram.q.pins[index]], INPUT);
	}
	// VCC, GND
	ic_pin_write(worker.package.pins[dram.signals[DRAM_GND]], LOW);
	ic_pin_write(worker.package.pins[dram.signals[DRAM_VCC]], HIGH);
	// Idle
	ic_pin_write(worker.package.pins[dram.signals[DRAM_WE]], HIGH);
	if (ic_dram_oe(dram)) {
		ic_pin_write(worker.package.pins[dram.signals[DRAM_OE]], HIGH);
	}
	ic_pin_write(worker.package.pins[dram.signals[DRAM_RAS]], HIGH);
	ic_pin_write(worker.package.pins[dram.signals[DRAM_CAS]], HIGH);
	for (unsigned int index = 0; index < dram.row.count; ++index) {
		ic_pin_write(worker.package.pins[dram.signals[DRAM_RAS]], LOW);
		ic_pin_write(worker.package.pins[dram.signals[DRAM_RAS]], HIGH);
	}
	// Test
	worker.indicator = 0;
	Debugln(F(RAM_FILL_01));
	ic_bus_data(dram.d, LOW, true);
	ic_dram_fill(dram, true);
	Debugln(F(RAM_FILL_10));
	ic_bus_data(dram.d, HIGH, true);
	ic_dram_fill(dram, true);
	Debugln(F(RAM_FILL_00));
	ic_bus_data(dram.d, LOW, false);
	ic_dram_fill(dram);
	Debugln(F(RAM_FILL_11));
	ic_bus_data(dram.d, HIGH, false);
	ic_dram_fill(dram);
	// Shutdown
	ic_pin_write(worker.package.pins[dram.signals[DRAM_VCC]], LOW);
#if TIME
	unsigned long stop = millis();
	Serial.print(F(TIME_ELAPSED));
	Serial.println((stop - start) / 1000.0);
#endif
}
#else
void ic_dram(DRAM& dram) {
	// UI
	worker.ram = 0;
	worker.color = TFT_WHITE;
	for (unsigned int index = 0; index < 4; ++index) {
		ui_draw_ram();
	}
	worker.ram = 0;
#if TIME
	// Timer
	unsigned long start = millis();
#endif
	// Setup
	dram.row.high = 1 << dram.row.count;
	dram.column.high = 1 << dram.column.count;
	for (unsigned int index = 0; index < dram.q.count; ++index) {
		PIN_MODE(worker.package.pins[dram.q.pins[index]], INPUT);
	}
	// VCC, GND
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_GND]], LOW);
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_VCC]], HIGH);
	// Idle
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_WE]], HIGH);
	if (ic_dram_oe(dram)) {
		DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_OE]], HIGH);
	}
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_RAS]], HIGH);
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_CAS]], HIGH);
	for (unsigned int index = 0; index < dram.row.count; ++index) {
		DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_RAS]], LOW);
		DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_RAS]], HIGH);
	}
	// Test
	worker.indicator = 0;
	Debugln(F(RAM_FILL_01));
	ic_bus_data(dram.d, LOW, true);
	ic_dram_fill(dram, true);
	Debugln(F(RAM_FILL_10));
	ic_bus_data(dram.d, HIGH, true);
	ic_dram_fill(dram, true);
	Debugln(F(RAM_FILL_00));
	ic_bus_data(dram.d, LOW, false);
	ic_dram_fill(dram);
	Debugln(F(RAM_FILL_11));
	ic_bus_data(dram.d, HIGH, false);
	ic_dram_fill(dram);
	// Shutdown
	DIGITAL_WRITE(worker.package.pins[dram.signals[DRAM_VCC]], LOW);
#if TIME
	unsigned long stop = millis();
	Serial.print(F(TIME_ELAPSED));
	Serial.println((stop - start) / 1000.0);
#endif
}
#endif

//////////
// SRAM //
//////////

#if FIO
void ic_sram_write(SRAM& sram) {
	// Address
	ic_bus_write(sram.address);
	ic_pin_write(worker.package.pins[sram.signals[SRAM_CS]], LOW);
	// WE
	ic_pin_write(worker.package.pins[sram.signals[SRAM_WE]], LOW);
	// D
	ic_bus_write(sram.d);
	// Idle
	ic_pin_write(worker.package.pins[sram.signals[SRAM_WE]], HIGH);
	ic_pin_write(worker.package.pins[sram.signals[SRAM_CS]], HIGH);
}

void ic_sram_read(SRAM& sram) {
	// Address
	ic_bus_write(sram.address);
	ic_pin_write(worker.package.pins[sram.signals[SRAM_CS]], LOW);
	// Q
	ic_bus_read(sram.q);
	// Idle
	ic_pin_write(worker.package.pins[sram.signals[SRAM_CS]], HIGH);
}
#else
void ic_sram_write(SRAM& sram) {
	// Address
	ic_bus_write(sram.address);
	DIGITAL_WRITE(worker.package.pins[sram.signals[SRAM_CS]], LOW);
	// WE
	DIGITAL_WRITE(worker.package.pins[sram.signals[SRAM_WE]], LOW);
	// D
	ic_bus_write(sram.d);
	// Idle
	DIGITAL_WRITE(worker.package.pins[sram.signals[SRAM_WE]], HIGH);
	DIGITAL_WRITE(worker.package.pins[sram.signals[SRAM_CS]], HIGH);
}

void ic_sram_read(SRAM& sram) {
	// Address
	ic_bus_write(sram.address);
	DIGITAL_WRITE(worker.package.pins[sram.signals[SRAM_CS]], LOW);
	// Q
	ic_bus_read(sram.q);
	// Idle
	DIGITAL_WRITE(worker.package.pins[sram.signals[SRAM_CS]], HIGH);
}
#endif

void ic_sram_fill(SRAM& sram, const bool alternate = false) {
	uint16_t mask = alternate ? dram.d.high : 0;
	worker.color = COLOR_GOOD;
	// Write full address space
	ic_bus_output(sram.d);
	for (sram.address.value = 0; sram.address.value < sram.address.high; ++sram.address.value) {
		ic_sram_write(sram);
		sram.d.value ^= mask;
	}
	// Read full address space
	ic_bus_input(sram.q);
	for (sram.address.value = 0; sram.address.value < sram.address.high; ++sram.address.value) {
		worker.success = 0;
		worker.failure = 0;
		ic_sram_read(sram);
		if (sram.d.value != sram.q.value) {
			Debug("A ");
			Debug(sram.address.value, BIN);
			Debug(", D ");
			Debug(sram.d.value, BIN);
			Debug(", Q ");
			Debug(sram.q.value, BIN);
			Debug(", delta ");
			Debugln(sram.d.value ^ sram.q.value, BIN);
			worker.failure++;
			worker.color = COLOR_BAD;
		} else {
			worker.success++;
		}
		sram.d.value ^= mask;
		ui_draw_indicator((worker.success ? (worker.failure ? COLOR_MIXED : COLOR_GOOD) : COLOR_BAD));
#if CAPTURE
		if (ts_touched()) {
			sd_screen_capture();
		}
#endif
	}
	ui_draw_ram();
}

#if FIO
void ic_sram(SRAM& sram) {
	// UI
	worker.ram = 0;
	worker.color = TFT_WHITE;
	for (unsigned int index = 0; index < 4; ++index) {
		ui_draw_ram();
	}
	worker.ram = 0;
#if TIME
	// Timer
	unsigned long start = millis();
#endif
	// Setup
	sram.address.high = 1 << sram.address.count;
	for (unsigned int index = 0; index < sram.q.count; ++index) {
		ic_pin_mode(worker.package.pins[sram.q.pins[index]], INPUT);
	}
	// VCC, GND
	ic_pin_write(worker.package.pins[sram.signals[SRAM_GND]], LOW);
	ic_pin_write(worker.package.pins[sram.signals[SRAM_VCC]], HIGH);
	// Idle
	ic_pin_write(worker.package.pins[sram.signals[SRAM_WE]], HIGH);
	ic_pin_write(worker.package.pins[sram.signals[SRAM_CS]], HIGH);
	// Test
	worker.indicator = 0;
	Debugln(F(RAM_FILL_01));
	ic_bus_data(sram.d, LOW, true);
	ic_sram_fill(sram, true);
	Debugln(F(RAM_FILL_10));
	ic_bus_data(sram.d, HIGH, true);
	ic_sram_fill(sram, true);
	Debugln(F(RAM_FILL_00));
	ic_bus_data(sram.d, LOW, false);
	ic_sram_fill(sram);
	Debugln(F(RAM_FILL_11));
	ic_bus_data(sram.d, HIGH, false);
	ic_sram_fill(sram);
	// Shutdown
	ic_pin_write(worker.package.pins[sram.signals[SRAM_VCC]], LOW);
#if TIME
	unsigned long stop = millis();
	Serial.print(F(TIME_ELAPSED));
	Serial.println((stop - start) / 1000.0);
#endif
}
#else
void ic_sram(SRAM& sram) {
	// UI
	worker.ram = 0;
	worker.color = TFT_WHITE;
	for (unsigned int index = 0; index < 4; ++index) {
		ui_draw_ram();
	}
	worker.ram = 0;
#if TIME
	// Timer
	unsigned long start = millis();
#endif
	// Setup
	sram.address.high = 1 << sram.address.count;
	for (unsigned int index = 0; index < sram.q.count; ++index) {
		PIN_MODE(worker.package.pins[sram.q.pins[index]], INPUT);
	}
	// VCC, GND
	DIGITAL_WRITE(worker.package.pins[sram.signals[SRAM_GND]], LOW);
	DIGITAL_WRITE(worker.package.pins[sram.signals[SRAM_VCC]], HIGH);
	// Idle
	DIGITAL_WRITE(worker.package.pins[sram.signals[SRAM_WE]], HIGH);
	DIGITAL_WRITE(worker.package.pins[sram.signals[SRAM_CS]], HIGH);
	// Test
	worker.indicator = 0;
	Debugln(F(RAM_FILL_01));
	ic_bus_data(sram.d, LOW, true);
	ic_sram_fill(sram, true);
	Debugln(F(RAM_FILL_10));
	ic_bus_data(sram.d, HIGH, true);
	ic_sram_fill(sram, true);
	Debugln(F(RAM_FILL_00));
	ic_bus_data(sram.d, LOW, false);
	ic_sram_fill(sram);
	Debugln(F(RAM_FILL_11));
	ic_bus_data(sram.d, HIGH, false);
	ic_sram_fill(sram);
	// Shutdown
	DIGITAL_WRITE(worker.package.pins[sram.signals[SRAM_VCC]], LOW);
#if TIME
	unsigned long stop = millis();
	Serial.print(F(TIME_ELAPSED));
	Serial.println((stop - start) / 1000.0);
#endif
}
#endif

//#pragma GCC pop_options

//////////////////
// Touch screen //
//////////////////

#define TS_MIN 10
#define TS_MAX 1000
#define TS_DELAY 10

bool ts_touched() {
	TSPoint point = ts.getPoint();
	PIN_MODE(YP, OUTPUT);
	PIN_MODE(XM, OUTPUT);
	DIGITAL_WRITE(YP, HIGH);
	DIGITAL_WRITE(XM, HIGH);
	if (point.z > TS_MIN && point.z < TS_MAX) {
		// TODO adapt code to your display
		worker.x = map(point.y, TS_LEFT, TS_RT, TFT_WIDTH, 0);
		worker.y = map(point.x, TS_TOP, TS_BOT, 0, TFT_HEIGHT);
		return true;
	} else {
		worker.x = 0xFFFF;
		worker.y = 0xFFFF;
	}
	return false;
}

void ts_wait() {
	bool loop = true;
	while (loop) {
		TSPoint point = ts.getPoint();
		if (point.z > TS_MIN && point.z < TS_MAX) {
			loop = false;
		} else {
			delay(TS_DELAY);
		}
	}
	PIN_MODE(YP, OUTPUT);
	PIN_MODE(XM, OUTPUT);
	DIGITAL_WRITE(YP, HIGH);
	DIGITAL_WRITE(XM, HIGH);
}

/////////
// TFT //
/////////

void tft_init() {
	tft.reset();
	uint16_t identifier = tft.readID();
	tft.begin(identifier);
	Debug(F(TFT_INITIALIZED));
	Debugln(identifier, HEX);
	// TODO adapt code to your display
	tft.setRotation(1);
}

/////////////
// SD card //
/////////////

#if CAPTURE

#define CAPTURE_KEYBOARD 0 // 1 to enable images on keyboard event,  0 to disable
#define CAPTURE_STATE 0 // 1 to enable images on state change,  0 to disable
#define CAPTURE_ERROR 0 // 1 to enable images on error,  0 to disable
#define CAPTURE_CONTENT 1 // 1 to enable images on content change,  0 to disable

void sd_screen_capture() {
	static unsigned int index = 0;
	String filename("image.");
	filename += index++;
	filename += ".data";
	Serial.print("Save ");
	Serial.print(filename);
	uint16_t* pixels = (uint16_t*)malloc(sizeof(uint16_t) * TFT_WIDTH);
	if (pixels) {
		SdFile file;
		if (file.open(filename.c_str(), O_CREAT | O_RDWR)) {
			for (unsigned int y = 0; y < TFT_HEIGHT; ++y) {
				tft.readGRAM(0, y, pixels, TFT_WIDTH, 1);
				file.write(pixels, sizeof(uint16_t) * TFT_WIDTH);
			}
		}
		file.close();
	} else {
		Serial.println(" failed");
	}
	free(pixels);
	Serial.println(" done");
}

#endif

////////////////////
// User interface //
////////////////////

void ui_draw_center(const __FlashStringHelper* text, int16_t y) {
	int16_t x;
	uint16_t width, height;
	tft.getTextBounds(text, 0, y, &x, &y, &width, &height);
	tft.setCursor((TFT_WIDTH - width) / 2, y);
	tft.println(text);
}

void ui_draw_wrap(const String &string, const unsigned int size) {
	unsigned int start = 0;
	unsigned int index = start;
	tft.setTextSize(size);
	while (index < string.length()) {
		if (string[index] == ' ') {
			if ((tft.getCursorX() + (index - start - 1) * 6 * size) > TFT_WIDTH) {
				tft.println();
			}
			tft.print(string.substring(start, index++));
			if (tft.getCursorX() + 6 * size < TFT_WIDTH) {
				tft.setCursor(tft.getCursorX() + 6 * size, tft.getCursorY());
			}
			start = index;
		}
		++index;
	}
	if (index != start) {
		if ((tft.getCursorX() + (index - start) * 6 * size) > TFT_WIDTH) {
			tft.println();
		}
		tft.println(string.substring(start, index));
	}
}

void ui_draw_error(const __FlashStringHelper* text) {
	tft.setTextSize(2);
	tft.setTextColor(TFT_RED);
	ui_draw_center(text, AREA_CONTENT + 32);
}

void ui_draw_indicator(const unsigned int color) {
	tft.fillRect(worker.indicator * 20 + 2, 222, 16, 16, color);
	worker.indicator++;
	worker.indicator %= 16;
	tft.fillRect(worker.indicator * 20 + 2, 222, 16, 16, COLOR_BACKGROUND);
}

void ui_draw_percentage() {
	int width = map(worker.success, 0, worker.success + worker.failure, 0, TFT_WIDTH - 2 * 2);
	tft.fillRect(2, 202, width, 16, COLOR_GOOD);
	tft.fillRect(2 + width, 202, TFT_WIDTH - 2 * 2 - width, 16, COLOR_BAD);
}

void ui_draw_ram() {
	unsigned int x = (worker.ram - 2) * RAM_BOX + (TFT_WIDTH / 2);
	tft.fillRect(x, RAM_TOP, RAM_SIZE, RAM_SIZE, worker.color);
	switch (worker.ram) {
	case 0: // 0 1
		tft.drawFastHLine(x, RAM_BOTTOM - 4, RAM_HALF, TFT_BLACK);
		tft.drawFastHLine(x + RAM_HALF, RAM_TOP + 4, RAM_HALF, TFT_BLACK);
		tft.drawFastVLine(x + RAM_HALF, RAM_TOP + 4, RAM_SIZE - 8, TFT_BLACK);
		break;
	case 1: // 1 0
		tft.drawFastHLine(x, RAM_TOP + 4, RAM_HALF, TFT_BLACK);
		tft.drawFastHLine(x + RAM_HALF, RAM_BOTTOM - 4, RAM_HALF, TFT_BLACK);
		tft.drawFastVLine(x + RAM_HALF, RAM_TOP + 4, RAM_SIZE - 8, TFT_BLACK);
		break;
	case 2: // 0
		tft.drawFastHLine(x, RAM_BOTTOM - 4, RAM_SIZE, TFT_BLACK);
		break;
	case 3: // 1
		tft.drawFastHLine(x, RAM_TOP + 4, RAM_SIZE, TFT_BLACK);
		break;
	}
	worker.ram++;
}

void ui_draw_header(bool erase) {
	if (erase) {
		tft.fillScreen(COLOR_BACKGROUND);
	}
	tft.setTextColor(COLOR_TEXT);
	tft.setTextSize(5);
	ui_draw_center(F("IC TESTER"), 2);
}

void ui_draw_enter() {
	buttons[button_enter].initButton(&tft, BUTTON_X + (BUTTON_W + BUTTON_SPACING_X) * 0.5,
			BUTTON_Y + 2 * (BUTTON_H + BUTTON_SPACING_Y), 2 * BUTTON_W + BUTTON_SPACING_X, BUTTON_H,
			COLOR_ENTER, COLOR_ENTER, COLOR_LABEL, (char*)"ENTER", BUTTON_TEXTSIZE);
	buttons[button_enter].drawButton();
}

void ui_draw_test() {
	buttons[button_test].initButton(&tft, BUTTON_X + (BUTTON_W + BUTTON_SPACING_X) * 0.5,
			BUTTON_Y + 2 * (BUTTON_H + BUTTON_SPACING_Y), 2 * BUTTON_W + BUTTON_SPACING_X, BUTTON_H,
			COLOR_ENTER, COLOR_ENTER, COLOR_LABEL, (char*)"TEST", BUTTON_TEXTSIZE);
	buttons[button_test].drawButton();
}

void ui_draw_redo() {
	buttons[button_redo].initButton(&tft, BUTTON_X + (BUTTON_W + BUTTON_SPACING_X) * 0.5,
			BUTTON_Y + 2 * (BUTTON_H + BUTTON_SPACING_Y), 2 * BUTTON_W + BUTTON_SPACING_X, BUTTON_H,
			COLOR_ENTER, COLOR_ENTER, COLOR_LABEL, (char*)"REDO", BUTTON_TEXTSIZE);
	buttons[button_redo].drawButton();
}

void ui_draw_escape() {
	buttons[button_escape].initButton(&tft, BUTTON_X + (BUTTON_W + BUTTON_SPACING_X) * 4,
			BUTTON_Y + 2 * (BUTTON_H + BUTTON_SPACING_Y), BUTTON_W, BUTTON_H,
			COLOR_ESCAPE, COLOR_ESCAPE, COLOR_LABEL, (char*)"ESC", BUTTON_TEXTSIZE);
	buttons[button_escape].drawButton();
}

void ui_draw_menu(const char* items[], const unsigned int count) {
	unsigned int y = AREA_CONTENT - 4;
	ui_draw_header(true);
	for (unsigned int index = 0; index < count; ++index) {
		buttons[index].initButton(&tft,
		BUTTON_X,
		BUTTON_Y + index * (BUTTON_H + BUTTON_SPACING_Y),
		BUTTON_W, BUTTON_H, COLOR_KEY, COLOR_KEY, COLOR_LABEL,
				(char*) String(index + 1).c_str(), BUTTON_TEXTSIZE);
		buttons[index].drawButton();
		tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
		tft.setTextSize(3);
		tft.setCursor(2 * BUTTON_X, y);
		y += BUTTON_H + BUTTON_SPACING_Y;
		tft.print(items[index]);
	}
}

void ui_draw_screen() {
	switch (worker.state) {
	case state_startup:
		ui_draw_header(true);
		// Version
		tft.setTextSize(2);
		ui_draw_center(F("version 1.3.0"), tft.getCursorY());
		ui_draw_center(F("14/09/2020"), tft.getCursorY());
		// Author
		tft.setTextColor(TFT_WHITE);
		ui_draw_center(F("Patrick Lafarguette"), AREA_CONTENT + 32);
		ui_draw_escape();
		break;
	case state_media:
		ui_draw_header(true);
		// Error
		ui_draw_error(F(INSERT_SD_CARD));
		ui_draw_escape();
		break;
	case state_menu: {
		worker.action = action_idle;
		const char* items[] = { IDENTIFY_LOGIC, TEST_LOGIC, TEST_RAM };
		ui_draw_menu(items, 3);
	}
#if CAPTURE
		sd_screen_capture();
#endif
		break;
	case state_identify_logic: {
		const char* items[] = { DIP14_PACKAGE, DIP16_PACKAGE };
		ui_draw_menu(items, 2);
		ui_draw_escape();
	}
		break;
	case state_package_dip14:
	case state_package_dip16:
		worker.action = action_identify_logic;
		worker.index = -1;
		ics.clear();
		ui_draw_header(true);
		// Action
		tft.setTextSize(2);
		ui_draw_center(F(IDENTIFY_LOGIC__), tft.getCursorY());
		ic_identify_logic();
		break;
	case state_keyboard:
		tft.fillScreen(TFT_NAVY);
		tft.drawRect(0, 0, TFT_WIDTH, INPUT_HEIGHT, TFT_YELLOW);
		tft.drawRect(1, 1, TFT_WIDTH - 2, INPUT_HEIGHT - 2, TFT_YELLOW);
		for (unsigned int row = 0; row < 2; ++row) {
			for (unsigned int column = 0; column < 5; ++column) {
				buttons[column + row * 5].initButton(&tft,
						BUTTON_X + column * (BUTTON_W + BUTTON_SPACING_X),
						BUTTON_Y + row * (BUTTON_H + BUTTON_SPACING_Y),
						BUTTON_W, BUTTON_H, COLOR_KEY, COLOR_KEY, COLOR_LABEL,
						(char*)String(column + row * 5).c_str(), BUTTON_TEXTSIZE);
				buttons[column + row * 5].drawButton();
			}
		}
		ui_draw_enter();
		buttons[button_clear].initButton(&tft, BUTTON_X + (BUTTON_W + BUTTON_SPACING_X) * 2.5,
				BUTTON_Y + 2 * (BUTTON_H + BUTTON_SPACING_Y), 2 * BUTTON_W + BUTTON_SPACING_X, BUTTON_H,
				COLOR_KEY, COLOR_KEY, COLOR_LABEL, (char*)"CLR", BUTTON_TEXTSIZE);
		buttons[button_clear].drawButton();
		buttons[button_del].initButton(&tft, BUTTON_X + (BUTTON_W + BUTTON_SPACING_X) * 4,
				BUTTON_Y + 2 * (BUTTON_H + BUTTON_SPACING_Y), BUTTON_W, BUTTON_H,
				COLOR_DELETE, COLOR_DELETE, COLOR_LABEL, (char*)"DEL", BUTTON_TEXTSIZE);
		buttons[button_del].drawButton();
		break;
	case state_test_logic:
		ics.clear();
		lines.clear();
		ui_draw_header(true);
		// Action
		tft.setTextSize(2);
		ui_draw_center(F(TEST_LOGIC_), tft.getCursorY());
		ic_test_logic();
		break;
	case state_test_ram:
		ui_draw_header(true);
		// Action
		tft.setTextSize(2);
		ui_draw_center(F(TEST_RAM_), tft.getCursorY());
		ic_test_ram();
		break;
	case state_identified:
		ui_clear_footer();
		if (ics.count()) {
			ui_draw_test();
			if (ics.count() > 1) {
				// Add navigator
				buttons[button_previous].initButton(&tft, BUTTON_X + 2 * (BUTTON_W + BUTTON_SPACING_X),
						BUTTON_Y + 2 * (BUTTON_H + BUTTON_SPACING_Y), BUTTON_W, BUTTON_H,
						COLOR_KEY, COLOR_KEY, COLOR_LABEL, (char*)"<", BUTTON_TEXTSIZE);
				buttons[button_previous].drawButton();
				buttons[button_next].initButton(&tft, BUTTON_X + 3 * (BUTTON_W + BUTTON_SPACING_X),
						BUTTON_Y + 2 * (BUTTON_H + BUTTON_SPACING_Y),  BUTTON_W, BUTTON_H,
						COLOR_KEY, COLOR_KEY, COLOR_LABEL, (char*)">", BUTTON_TEXTSIZE);
				buttons[button_next].drawButton();
			}

		} else {
			ui_draw_content();
		}
		ui_draw_escape();
#if CAPTURE
		sd_screen_capture();
#endif
		break;
	case state_tested:
		ui_clear_footer();
		if (worker.found) {
			switch (worker.action) {
				case action_test_logic: {
					unsigned int total = worker.success + worker.failure;
					double percent = (100.0 * worker.success) / total;
					tft.print(total);
					tft.print(F(CYCLES));
					tft.print(percent);
					tft.println(F(PASSED));
					tft.setTextSize(2);
					tft.setTextColor(worker.success ? (worker.failure ? COLOR_MIXED : COLOR_GOOD) : COLOR_BAD);
					tft.println(worker.success ? (worker.failure ? F(UNRELIABLE) : F(GOOD)) : F(BAD));
				}
					break;
				case action_test_ram:
					break;
			}
			ui_draw_redo();
		} else {
			ui_draw_content();
		}
		ui_draw_escape();
#if CAPTURE
		sd_screen_capture();
#endif
		break;
	}
}

void ui_clear_content() {
	tft.fillRect(0, AREA_CONTENT, TFT_WIDTH, AREA_FOOTER - AREA_CONTENT, COLOR_BACKGROUND);
	tft.setCursor(0, AREA_CONTENT);
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(2);
}

void ui_clear_footer() {
	tft.fillRect(0, AREA_FOOTER, TFT_WIDTH, TFT_HEIGHT - AREA_FOOTER, COLOR_BACKGROUND);
}

void ui_draw_content() {
	switch (worker.action) {
	case action_identify_logic:
		ui_clear_content();
		switch (ics.count()) {
		case 0: // No match found
			ui_draw_error(F(NO_MATCH_FOUND));
			return;
		case 1:
			tft.println(F(MATCH_FOUND));
			break;
		default:
			tft.print(F(MATCH_FOUND));
			tft.print(" ");
			tft.print(worker.index + 1);
			tft.print("/");
			tft.println(ics.count());
			break;
		}
		break;
	case action_test_logic:
	case action_test_ram:
		if (worker.found) {
		} else {
			ui_draw_error(F(NO_MATCH_FOUND));
			return;
		}
		break;
	}
	tft.setCursor(0, tft.getCursorY() + 4);
	tft.setTextColor(COLOR_CODE);
	tft.setTextSize(3);
	tft.println(ics[worker.index]->code);
	tft.setCursor(0, tft.getCursorY() + 4);
	tft.setTextColor(COLOR_DESCRIPTION);
	ui_draw_wrap(ics[worker.index]->description, 2);
#if CAPTURE_CONTENT
	sd_screen_capture();
#endif
}

////////////////////////
// Integrated circuit //
////////////////////////

#if FIO
void ic_package_create() {
	for (uint8_t index = 0, delta = 0; index < worker.package.count; ++index) {
		if (index == worker.package.count >> 1) {
			delta = ZIF_COUNT - worker.package.count;
		}
		uint8_t port = digitalPinToPort(ZIF[index + delta]);
		worker.package.pins[index].bit = digitalPinToBitMask(ZIF[index + delta]);
		worker.package.pins[index].mode = portModeRegister(port);
		worker.package.pins[index].output = portOutputRegister(port);
		worker.package.pins[index].input = portInputRegister(port);
	}
}

void ic_package_output() {
	for (unsigned int index = 0; index < worker.package.count; ++index) {
		ic_pin_mode(worker.package.pins[index], OUTPUT);
		ic_pin_write(worker.package.pins[index], LOW);
	}
}

void ic_package_idle() {
	for (unsigned int index = 0; index < worker.package.count; ++index) {
		ic_pin_mode(worker.package.pins[index], INPUT);
	}
}
#else
void ic_package_create() {
	for (uint8_t index = 0, delta = 0; index < worker.package.count; ++index) {
		if (index == worker.package.count >> 1) {
			delta = ZIF_COUNT - worker.package.count;
		}
		worker.package.pins[index] = ZIF[index + delta];
	}
}

void ic_package_output() {
	for (unsigned int index = 0; index < worker.package.count; ++index) {
		PIN_MODE(worker.package.pins[index], OUTPUT);
	}
}
#endif

#if TEST
void ic_package_dump() {
	Serial.print("DIP");
	Serial.print(worker.package.count);
	Serial.print(" ");
	for (unsigned int index = 0; index < worker.package.count; ++index) {
		if (index) {
			Serial.print(", ");
		}
		Serial.print(worker.package.pins[index]);
	}
	Serial.println();
}

void ic_package_test() {
	worker.package.count = 20;
	ic_package_create();
	// 30, 32, 34, 36, 38, 40, 42, 44, 50, 48, 49, 47, 45, 43, 41, 39, 37, 35, 33, 31
	ic_package_dump();
	worker.package.count = 18;
	ic_package_create();
	// 30, 32, 34, 36, 38, 40, 42, 44, 50, 47, 45, 43, 41, 39, 37, 35, 33, 31
	ic_package_dump();
	worker.package.count = 16;
	ic_package_create();
	// 30, 32, 34, 36, 38, 40, 42, 44, 45, 43, 41, 39, 37, 35, 33, 31
	// 30, 32, 34, 36, 38, 40, 42, 44, 45, 43, 41, 39, 37, 35, 33, 31
	ic_package_dump();
	worker.package.count = 14;
	ic_package_create();
	// 30, 32, 34, 36, 38, 40, 42, 43, 41, 39, 37, 35, 33, 31
	// 30, 32, 34, 36, 38, 40, 42, 43, 41, 39, 37, 35, 33, 31
	ic_package_dump();
}
#endif

#if FIO
bool ic_test_logic(String line) {
	bool result = true;
	int8_t clock = -1;
	Debugln(line);
	// VCC, GND and output
	for (unsigned int index = 0; index < worker.package.count; ++index) {
		switch (line[index]) {
		case 'V':
			ic_pin_mode(worker.package.pins[index], OUTPUT);
			ic_pin_write(worker.package.pins[index], HIGH);
			break;
		case 'G':
			ic_pin_mode(worker.package.pins[index], OUTPUT);
			ic_pin_write(worker.package.pins[index], LOW);
			break;
		case 'L':
			ic_pin_write(worker.package.pins[index], LOW);
			ic_pin_mode(worker.package.pins[index], INPUT_PULLUP);
			break;
		case 'H':
			ic_pin_write(worker.package.pins[index], LOW);
			ic_pin_mode(worker.package.pins[index], INPUT_PULLUP);
			break;
		}
	}
	delay(5);
	// Write
	for (unsigned int index = 0; index < worker.package.count; ++index) {
		switch (line[index]) {
		case 'X':
		case '0':
			ic_pin_mode(worker.package.pins[index], OUTPUT);
			ic_pin_write(worker.package.pins[index], LOW);
			break;
		case '1':
			ic_pin_mode(worker.package.pins[index], OUTPUT);
			ic_pin_write(worker.package.pins[index], HIGH);
			break;
		case 'C':
			clock = index;
			ic_pin_mode(worker.package.pins[index], OUTPUT);
			ic_pin_write(worker.package.pins[index], LOW);
			break;
		}
	}
	// Clock
	if (clock != -1) {
		ic_pin_mode(worker.package.pins[clock], INPUT_PULLUP);
		delay(10);
		ic_pin_mode(worker.package.pins[clock], OUTPUT);
		ic_pin_write(worker.package.pins[clock], LOW);
	}
	delay(5);
	// Read
	for (unsigned int index = 0; index < worker.package.count; ++index) {
		switch (line[index]) {
		case 'H':
			if (!ic_pin_read(worker.package.pins[index])) {
				result = false;
				Debug(F(_LOW));
			} else {
				Debug(F(SPACE));
			}
			break;
		case 'L':
			if (ic_pin_read(worker.package.pins[index])) {
				result = false;
				Debug(F(_HIGH));
			} else {
				Debug(F(SPACE));
			}
			break;
		default:
			Debug(F(SPACE));
		}
	}
	Debugln();
	return result;
}
#else
bool ic_test_logic(String line) {
	bool result = true;
	int clock = -1;
	Debugln(line);
	// VCC, GND and output
	for (unsigned int index = 0; index < worker.package.count; ++index) {
		switch (line[index]) {
		case 'V':
			PIN_MODE(worker.package.pins[index], OUTPUT);
			DIGITAL_WRITE(worker.package.pins[index], HIGH);
			break;
		case 'G':
			PIN_MODE(worker.package.pins[index], OUTPUT);
			DIGITAL_WRITE(worker.package.pins[index], LOW);
			break;
		case 'L':
			DIGITAL_WRITE(worker.package.pins[index], LOW);
			PIN_MODE(worker.package.pins[index], INPUT_PULLUP);
			break;
		case 'H':
			DIGITAL_WRITE(worker.package.pins[index], LOW);
			PIN_MODE(worker.package.pins[index], INPUT_PULLUP);
			break;
		}
	}
	delay(5);
	// Write
	for (unsigned int index = 0; index < worker.package.count; ++index) {
		switch (line[index]) {
		case 'X':
		case '0':
			PIN_MODE(worker.package.pins[index], OUTPUT);
			DIGITAL_WRITE(worker.package.pins[index], LOW);
			break;
		case '1':
			PIN_MODE(worker.package.pins[index], OUTPUT);
			DIGITAL_WRITE(worker.package.pins[index], HIGH);
			break;
		case 'C':
			clock = worker.package.pins[index];
			PIN_MODE(worker.package.pins[index], OUTPUT);
			DIGITAL_WRITE(worker.package.pins[index], LOW);
			break;
		}
	}
	// Clock
	if (clock != -1) {
		PIN_MODE(clock, INPUT_PULLUP);
		delay(10);
		PIN_MODE(clock, OUTPUT);
		DIGITAL_WRITE(clock, LOW);
	}
	delay(5);
	// Read
	for (unsigned int index = 0; index < worker.package.count; ++index) {
		switch (line[index]) {
		case 'H':
			if (!DIGITAL_READ(worker.package.pins[index])) {
				result = false;
				Debug(F(_LOW));
			} else {
				Debug(F(SPACE));
			}
			break;
		case 'L':
			if (DIGITAL_READ(worker.package.pins[index])) {
				result = false;
				Debug(F(_HIGH));
			} else {
				Debug(F(SPACE));
			}
			break;
		default:
			Debug(F(SPACE));
		}
	}
	Debugln();
	return result;
}
#endif

void ic_identify_logic() {
	String line;
	IC ic;
	File file = fat.open(FILENAME);
	if (file) {
		worker.indicator= 0;
		while (file.available()) {
			file.readStringUntil('$');
			if (!file.available()) {
				// Avoid timeout at the end of file
				break;
			}
			ic.code = file.readStringUntil('\n');
			ic.description = file.readStringUntil('\n');
			ic.count = file.readStringUntil('\n').toInt();
			if (worker.package.count == ic.count) {
				worker.ok = true;
				while (file.peek() != '$') {
					line = file.readStringUntil('\n');
					line.trim();
					if (ic_test_logic(line) == false) {
						worker.ok = false;
						break;
					}
					delay(50);
				}
				if (worker.ok) {
					tft.fillRect(ics.count() * 20 + 2, 202, 16, 16, COLOR_OK);
					ui_draw_indicator(COLOR_OK);
					ics.push(new IC(ic));
					worker.index++;
					ui_draw_content();
				} else {
					ui_draw_indicator(COLOR_KO);
				}

			} else {
				ui_draw_indicator(COLOR_SKIP);
			}
#if CAPTURE
			if (ts_touched()) {
				sd_screen_capture();
			}
#endif
		}
		file.close();
		worker.state = state_identified;
	} else {
		worker.state = state_media;
	}
	ui_draw_screen();
}

void ic_test_logic() {
	String line;
	IC ic;
	File file = fat.open(FILENAME);
	if (file) {
		worker.indicator = 0;
		worker.found = false;
		while (file.available()) {
			file.readStringUntil('$');
			if (!file.available()) {
				// Avoid timeout at the end of file
				break;
			}
			ic.code = file.readStringUntil('\n');
			ic.description = file.readStringUntil('\n');
			ic.count = file.readStringUntil('\n').toInt();
			if (worker.code.toInt() == ic.code.toInt()) {
				worker.found = true;
				worker.index = 0;
				worker.success = 0;
				worker.failure = 0;
				worker.package.count = ic.count;
				ic_package_create();
				ics.push(new IC(ic));
				ui_draw_content();
				while (file.peek() != '$') {
					line = file.readStringUntil('\n');
					line.trim();
					lines.push(new String(line));
				}
				break;
			} else {
				ui_draw_indicator(COLOR_SKIP);
			}
#if CAPTURE
			if (ts_touched()) {
				sd_screen_capture();
			}
#endif
		}
		file.close();
		if (worker.found) {
			bool loop = true;
			while (loop) {
				worker.ok = true;
				for (unsigned int index = 0; index < lines.count(); ++index) {
					if (ic_test_logic(*lines[index]) == false) {
						worker.ok = false;
					}
					delay(50);
				}
				if (worker.ok) {
					worker.success++;
					ui_draw_indicator(COLOR_GOOD);
				} else {
					worker.failure++;
					ui_draw_indicator(COLOR_BAD);
				}
				ui_draw_percentage();
				loop = !ts_touched();
			}
#if CAPTURE
			sd_screen_capture();
#endif
		}
		worker.state = state_tested;
	} else {
		worker.state = state_media;
	}
	ui_draw_screen();
}

void ic_test_dram(IC& ic) {
	switch (dram_identify(worker.code.toInt())) {
	case DRAM_4164:
		// Code base
		// Time elapsed 100.43

		// RAS inner loop
		// Time elapsed 97.55

		// DIO2 library
		// Time elapsed 70.31

		// uint8_t in ic_bus_xx functions
		// Time elapsed 68.93

		// -o2
		// Time elapsed 61.40

		// DATA[] in ic_bus_xx functions
		// Time elapsed 60.70

		// FIO and DIO2
		// Time elapsed 54.80

		// FIO
		// Time elapsed 54.80
		ic.type = TYPE_DRAM;
		ic.description = DRAM_64K_X1;
		dram = dram41xx;
		dram.row.count = 8;
		dram.column.count = 8;
		break;
	case DRAM_41256:
		// FIO
		// Time elapsed 233.72
		ic.type = TYPE_DRAM;
		ic.description = DRAM_256K_X1;
		dram = dram41xx;
		dram.row.count = 9;
		dram.column.count = 9;
		break;
	case DRAM_4416:
		// DATA[] in ic_bus_xx functions
		// Time elapsed 15.82

		// FIO and DIO2
		// Time elapsed 14.15

		// FIO
		// Time elapsed 14.15
		ic.type = TYPE_DRAM;
		ic.description = DRAM_16K_X4;
		dram = dram4416;
		break;
	case DRAM_44256:
		// Code base
		// Time elapsed 545.19

		// RAS inner loop
		// Time elapsed 495.35

		// DIO2 library
		// Time elapsed 353.53

		// uint8_t in ic_bus_xx functions
		// Time elapsed 346.86

		// -O2
		// Time elapsed 306.70

		// DATA[] in ic_bus_xx functions
		// Time elapsed 297.28

		// FIO and DIO2
		// Time elapsed 264.58

		// FIO
		// Time elapsed 264.59
		ic.type = TYPE_DRAM;
		ic.description = DRAM_256K_X4;
		dram = dram44256;
		break;
	default:
		break;
	}
}

void ic_test_sram(IC& ic) {
	switch (sram_identify(worker.code.toInt())) {
	case SRAM_2114:
		// FIO
		// Time elapsed 10.97
		ic.type = TYPE_SRAM;
		ic.description = SRAM_1K_X4;
		sram = sram2114;
		break;
	default:
		break;
	}
}

void ic_test_ram() {
	IC ic;
	ic.code = worker.code;

	worker.found = true;
	worker.index = 0;
	ic_test_dram(ic);
	ic_test_sram(ic);
	if (ic.type != TYPE_NONE) {
		worker.found = true;
		worker.package.count = ic.type == TYPE_DRAM ? dram.count : sram.count;
		ic_package_create();
		ic_package_output();
		ics.push(new IC(ic));
		ui_draw_content();
		switch (ic.type) {
		case TYPE_DRAM:
			ic_dram(dram);
			break;
		case TYPE_SRAM:
			ic_sram(sram);
			break;
		}
		ic_package_idle();
	}
	worker.state = state_tested;
	ui_draw_screen();
}

/////////////
// Arduino //
/////////////

void setup() {
	Serial.begin(115200);
#if TEST
	ic_package_test();
	while (true) {};
#endif
	// TFT setup
	tft_init();
	// SD card
	worker.state = fat.begin(10) ? state_startup : state_media;
	// UI
	ui_draw_screen();
}

void loop() {
	bool touched = ts_touched();
	int state = worker.state;
	if (touched) {
		switch (worker.state) {
		case state_startup:
		case state_identified:
		case state_tested:
			switch (worker.action) {
				case action_identify_logic:
					if (buttons[button_test].contains(worker.x, worker.y)) {
						Debugln(F("test"));
						worker.code = ics[worker.index]->code;
						worker.action = action_test_logic;
						state =  state_test_logic;
					}
					break;
				case action_test_logic:
				case action_test_ram:
					if (buttons[button_redo].contains(worker.x, worker.y)) {
						Debugln(F("redo"));
						state = worker.action == action_test_logic ? state_test_logic : state_test_ram;
					}
					break;
			}
			if (buttons[button_escape].contains(worker.x, worker.y)) {
				Debugln(F("menu"));
				state = state_menu;
			}
			break;
		case state_media:
			if (buttons[button_escape].contains(worker.x, worker.y)) {
				Debugln(F("media"));
				if (fat.begin(10)) {
					state = state_menu;
				} else {
					Debugln(F("SD error"));
				}
			}
			break;
		case state_menu:
			if (buttons[button_identify_logic].contains(worker.x, worker.y)) {
				Debugln(F("identify logic"));
				state = state_identify_logic;
			}
			if (buttons[button_test_logic].contains(worker.x, worker.y)) {
				Debugln(F("test logic"));
				ics.clear();
				worker.action = action_test_logic;
				worker.code = "";
				state = state_keyboard;
			}
			if (buttons[button_test_ram].contains(worker.x, worker.y)) {
				Debugln(F("test ram"));
				ics.clear();
				worker.action = action_test_ram;
				worker.code = "";
				state = state_keyboard;
			}
			break;
		case state_identify_logic:
			if (buttons[button_dip14].contains(worker.x, worker.y)) {
				Debugln(F("dip 14"));
				state = state_package_dip14;
				worker.package.count = 14;
				ic_package_create();
			}
			if (buttons[button_dip16].contains(worker.x, worker.y)) {
				Debugln(F("dip 16"));
				state = state_package_dip16;
				worker.package.count = 16;
				ic_package_create();
			}
			if (buttons[button_escape].contains(worker.x, worker.y)) {
				Debugln(F("menu"));
				state = state_menu;
			}
			break;
		}
	}
	if (worker.state == state) {
		switch (worker.state) {
		case state_keyboard:
			for (unsigned int index = 0; index < button_count; ++index) {
				if (buttons[index].contains(worker.x, worker.y)) {
					buttons[index].press(true);
				} else {
					buttons[index].press(false);
				}
				if (buttons[index].justReleased()) {
					buttons[index].drawButton();
#if CAPTURE_KEYBOARD
					sd_screen_capture();
#endif
				}
				if (buttons[index].justPressed()) {
					buttons[index].drawButton(true);
					// 0..9
					if (index < button_enter) {
						if (worker.code.length() < TEXT_LENGTH) {
							worker.code += String(index);
						}
						tft.setCursor(TEXT_X, TEXT_Y);
						tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
						tft.setTextSize(TEXT_SIZE);
						tft.print(worker.code);
					}
					// Enter
					if (index == button_enter) {
						if (worker.code.length()) {
							switch (worker.action) {
							case action_test_logic:
								state = state_test_logic;
								break;
							case action_test_ram:
								state = state_test_ram;
								break;
							}
						} else {
							state = state_menu;
						}
#if CAPTURE_STATE
						buttons[button_enter].drawButton();
#endif
					}
					// Clear
					if (index == button_clear) {
						tft.fillRect(TEXT_X, TEXT_Y, TFT_WIDTH - 2 - TEXT_X, TEXT_SIZE * 8, COLOR_BACKGROUND);
						worker.code = "";
					}
					// Delete
					if (index == button_del) {
						worker.code.remove(worker.code.length() - 1, 1);
						tft.fillRect(TEXT_X + (TEXT_SIZE * 6 * worker.code.length()), TEXT_Y, TEXT_SIZE * 6, TEXT_SIZE * 8, COLOR_BACKGROUND);
					}
					// UI debounce
					delay(100);
				}
			}
			break;
		case state_identified:
			if (ics.count() > 1) {
				for (unsigned int index = 0; index < button_escape; ++index) {
					if (buttons[index].contains(worker.x, worker.y)) {
						buttons[index].press(true);
					} else {
						buttons[index].press(false);
					}
					if (buttons[index].justReleased()) {
						buttons[index].drawButton();
#if CAPTURE_KEYBOARD
						sd_screen_capture();
#endif
					}
					if (buttons[index].justPressed()) {
						buttons[index].drawButton(true);
						// Previous
						if (index == button_previous) {
							if (worker.index == 0) {
								worker.index = ics.count();
							}
							worker.index--;
						}
						// Next
						if (index == button_next) {
							worker.index++;
							if (worker.index == ics.count()) {
								worker.index = 0;
							}
						}
						ui_draw_content();
						// UI debounce
						delay(100);
					}
				}
			}
			break;
		}
	}
	if (worker.state != state) {
#if CAPTURE_STATE
		sd_screen_capture();
#endif
		worker.state = state;
		ui_draw_screen();
		Debug(F("State: "));
		Debugln(worker.state);
	}
}
