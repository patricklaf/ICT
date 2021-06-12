// ICT
// © 2020-2021 Patrick Lafarguette

#include "ICT.h"

#include <MCUFRIEND_kbv.h>
#include <SdFat.h>
#include <TouchScreen.h>

#include "IC.h"
#include "Board.h"
#include "Keyboard.h"
#include "Memory.h"
#include "Stack.h"

// Set one of the language to 1.
// French UI, currently not viable as screen only supports
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
#if SD_FAT_VERSION < 20000
// Modify SdFatConfig.h
//
//#define ENABLE_SOFTWARE_SPI_CLASS 1
SdFatSoftSpi<12, 11, 13> fat;
#else
// Modify SdFatConfig.h
//
// #define SPI_DRIVER_SELECT 2
const uint8_t SD_CS_PIN = 10;
const uint8_t SOFT_MISO_PIN = 12;
const uint8_t SOFT_MOSI_PIN = 11;
const uint8_t SOFT_SCK_PIN  = 13;

SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> spi;

#define SDCONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(0), &spi)

#endif

SdFat fat;
File file;

IC ic;
Stack<IC*> ics;
Stack<String*> lines;
Stack<Filename*> folders;
Stack<Filename*> files;

String path;

Tiles TILES_MENU = {
		2,
		1,
		2,
		{ { TILE_FLAG_CENTER, "logic"}, { TILE_FLAG_CENTER, "memory" } }
};

Tiles TILES_LOGIC = {
		10,
		4,
		3,
		{
				{ TILE_FLAG_CENTER, "14"}, { TILE_FLAG_CENTER, "16" }, { TILE_FLAG_CENTER, "18" },
				{ TILE_FLAG_CENTER, "20"}, { TILE_FLAG_CENTER, "22" }, { TILE_FLAG_CENTER, "24" },
				{ TILE_FLAG_CENTER, "28"}, { TILE_FLAG_CENTER, "32" }, { TILE_FLAG_CENTER, "40" },
				{ TILE_FLAG_CENTER, "Keyboard"}
		}
};

uint8_t PIN_COUNT[] = { 14, 16, 18, 20, 22, 24, 28, 32, 40 };

Tiles TILES_ROM = {
		3,
		3,
		1,
 		{ { TILE_FLAG_NONE, "Break on blank fail" }, { TILE_FLAG_NONE, "Read to serial" }, { TILE_FLAG_NONE, "Read to file" } }
};

Tiles TILES_NONE = {
		4,
		4,
		1,
		{}
};

Tiles TILES_FRAM = {
		4,
		4,
		1,
		{
				{ TILE_FLAG_NONE, "Read to serial" },
				{ TILE_FLAG_NONE, "Read to file" },
				{ TILE_FLAG_NONE, "Erase" },
				{ TILE_FLAG_NONE, "Write" }
		}
};

Tiles TILES_BROWSE = {
		4,
		4,
		1,
		{
				{ TILE_FLAG_NONE, NULL },
				{ TILE_FLAG_NONE, NULL },
				{ TILE_FLAG_NONE, NULL },
				{ TILE_FLAG_NONE, NULL }
		}
};

bool options[option_count];

#define TEST 0 // 1 to enable test cases, 0 to disable
#define TIME 1 // 1 to enable time, 0 to disable
#define DEBUG 1 // 1 to enable serial debug messages, 0 to disable
#define MISMATCH 1 // 1 to enable serial mismatch messages, 0 to disable
#define CAPTURE 1 // 1 to enable screen captures, 0 to disable

#if DEBUG
#define Debug(...) Serial.print(__VA_ARGS__)
#define Debugln(...) Serial.println(__VA_ARGS__)
#else
#define Debug(...)
#define Debugln(...)
#endif

#define HEAP_BEGIN int32_t memory = freemem();
#define HEAP_END if (memory != freemem()) { Debug("LEAK "); Debugln(__func__); }
#define HEAP_FREE { Debug("HEAP "); Debug(__func__);  Debug(" ");  Debugln(freemem()); }

typedef struct Worker {
	uint8_t task;
	uint8_t screen;
	uint8_t action;
	bool interrupted;
	// Browse
	uint16_t first;
	uint16_t last;
	// TFT
	int x;
	int y;
	// File
	uint32_t position;
	// IC
	Package package;
	String code;
	// Identify
	uint8_t index;
	// Test
	bool found;
	bool ok;
	unsigned int success;
	unsigned int failure;
	// UI
	uint16_t color;
	unsigned int indicator;
	uint8_t ram;
	uint16_t content;
} Worker;

Worker worker;

Keyboard keyboard(&tft, 0, INPUT_HEIGHT + 1);

Board board(&tft, 0, AREA_CONTENT, TFT_WIDTH, AREA_FOOTER - AREA_CONTENT);

Adafruit_GFX_Button buttons[button_count];

// Filenames
#define INIFILE "ict.ini"
#define TMPFILE "ict.tmp"
#define LOGICFILE "logic.ict"
#define MEMORYFILE "memory.ict"

// Number format
void bin(uint32_t value, uint8_t count) {
	for (uint8_t index = count; index > 0;) {
		Debug(value >> (--index * 1) & 0x01,  BIN);
	}
}

void hexa(uint32_t value, uint8_t count) {
	for (uint8_t index = count; index > 0;) {
		Debug(value >> (--index * 4) & 0x0F, HEX);
	}
}

////////////
// Memory //
////////////

extern char *__brkval;

int freemem() {
  char top;
  return &top - __brkval;
}

/////////
// Bus //
/////////

const uint8_t BYTES[] = {
		(uint8_t)(1 << 0),
		(uint8_t)(1 << 1),
		(uint8_t)(1 << 2),
		(uint8_t)(1 << 3),
		(uint8_t)(1 << 4),
		(uint8_t)(1 << 5),
		(uint8_t)(1 << 6),
		(uint8_t)(1 << 7)
};

const uint16_t WORDS[] = {
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

void ic_bus_write_address_word(Bus& bus) {
	uint16_t lsb = bus.address;
	for (uint8_t index = 0; index < bus.width; ++index) {
		ic_pin_write(worker.package.pins[bus.pins[index]], lsb & WORDS[index] ? HIGH : LOW);
	}
}

void ic_bus_write_address_dword(Bus& bus) {
	uint16_t lsb = bus.address;
	uint16_t msb = bus.address >> 16;
	uint8_t index = 0;
	for (; index < 16; ++index) {
		ic_pin_write(worker.package.pins[bus.pins[index]], lsb & WORDS[index] ? HIGH : LOW);
	}
	for (; index < bus.width; ++index) {
		ic_pin_write(worker.package.pins[bus.pins[index]], msb & WORDS[index & 0x0F] ? HIGH : LOW);
	}
}

void ic_bus_write_data(Bus& bus) {
	for (uint8_t index = 0; index < bus.width; ++index) {
		ic_pin_write(worker.package.pins[bus.pins[index]], bus.data & BYTES[index]);
	}
}

void ic_bus_read_data(Bus& bus) {
	bus.data = 0;
	for (uint8_t index = 0; index < bus.width; ++index) {
		if (ic_pin_read(worker.package.pins[bus.pins[index]])) {
			bus.data |= BYTES[index];
		}
	}
}

void ic_bus_output(Bus& bus) {
	for (uint8_t index = 0; index < bus.width; ++index) {
		ic_pin_mode(worker.package.pins[bus.pins[index]], OUTPUT);
	}
}

void ic_bus_input(Bus& bus) {
	for (uint8_t index = 0; index < bus.width; ++index) {
		ic_pin_mode(worker.package.pins[bus.pins[index]], INPUT);
	}
}

void ic_bus_data(Bus& bus, uint8_t bit, const bool alternate) {
	uint8_t mask = alternate ? HIGH : LOW;
	bus.data = 0;
	for (uint8_t index = 0; index < bus.width; ++index) {
		if (bit) {
			bus.data |= BYTES[index];
		}
		bit ^= mask;
	}
	Debug(F("fill 0b"));
	bin(bus.data, bus.width);
	Debugln();
}

////////////
// Memory //
////////////

bool ic_memory_signal(uint8_t signal) {
	return memory.signals[signal] > -1;
}

/////////
// RAM //
/////////

void ic_ram_loop() {
	// UI
	worker.ram = 0;
	worker.color = TFT_WHITE;
	for (uint8_t index = 0; index < 4; ++index) {
		ui_draw_ram();
	}
	worker.ram = 0;
#if TIME
	// Timer
	unsigned long start = millis();
#endif
	// Setup
	for (uint8_t index = 0; index < memory.bus[BUS_Q].width; ++index) {
		ic_pin_mode(worker.package.pins[memory.bus[BUS_Q].pins[index]], INPUT);
	}
	// VCC, GND
	if ((memory.signals[SIGNAL_VBB] == 0) && (memory.signals[SIGNAL_VDD] == 7)) {
		// Power rails to -5 and +12 V
		Debugln("Enable -5 and +12 V");
		digitalWrite(A7, HIGH);
	}
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_GND]], LOW);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_VCC]], HIGH);
	// At package creation all pins are output low
	// LOW bus is preset, so only set HIGH bus high
	for (uint8_t index = 0; index < memory.bus[BUS_HIGH].width; ++index) {
		ic_pin_write(worker.package.pins[memory.bus[BUS_HIGH].pins[index]], HIGH);
	}
	// Idle
	memory.idle();
	// Test
	worker.indicator = 0;
	for (uint8_t loop = 0; loop < 4; ++loop) {
		// 0 00 0 false true
		// 1 01 0 false true
		// 2 10 1 true false
		// 3 11 1 true false
		bool alternate = !(loop >> 1);
		// 0 00 0 LOW
		// 1 01 1 HIGH
		// 2 10 0 LOW
		// 3 11 1 HIGH
		ic_bus_data(memory.bus[BUS_D], loop & 0x01, alternate);
		memory.fill(alternate);
	}
	// Shutdown
	if ((memory.signals[SIGNAL_VBB] == 0) && (memory.signals[SIGNAL_VDD] == 7)) {
		Debugln("Disable -5 and +12 V");
		// Power rails to TTL
		digitalWrite(A7, LOW);
	}
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_VCC]], LOW);
#if TIME
	unsigned long stop = millis();
	Serial.print(F(TIME_ELAPSED));
	Serial.println((stop - start) / 1000.0);
#endif
}


//////////
// DRAM //
//////////

void ic_dram_idle() {
	// Idle
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_WE]], HIGH);
	if (ic_memory_signal(SIGNAL_OE)) {
		ic_pin_write(worker.package.pins[memory.signals[SIGNAL_OE]], HIGH);
	}
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_RAS]], HIGH);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_CAS]], HIGH);
	for (uint8_t index = 0; index < memory.bus[BUS_RAS].width; ++index) {
		ic_pin_write(worker.package.pins[memory.signals[SIGNAL_RAS]], LOW);
		ic_pin_write(worker.package.pins[memory.signals[SIGNAL_RAS]], HIGH);
	}
}

void ic_dram_write() {
	// Row
	memory.write_address(memory.bus[BUS_RAS]);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_RAS]], LOW);
	// WE
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_WE]], LOW);
	// D
	ic_bus_write_data(memory.bus[BUS_D]);
	// Column
	memory.write_address(memory.bus[BUS_CAS]);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_CAS]], LOW);
	// Idle
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_WE]], HIGH);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_RAS]], HIGH);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_CAS]], HIGH);
}

void ic_dram_read() {
	// Row
	memory.write_address(memory.bus[BUS_RAS]);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_RAS]], LOW);
	// Column
	memory.write_address(memory.bus[BUS_CAS]);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_CAS]], LOW);
	// OE
	bool oe = ic_memory_signal(SIGNAL_OE);
	if (oe) {
		ic_pin_write(worker.package.pins[memory.signals[SIGNAL_OE]], LOW);
	}
	// Q
	ic_bus_read_data(memory.bus[BUS_Q]);
	// Idle
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_RAS]], HIGH);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_CAS]], HIGH);
	// OE
	if (oe) {
		ic_pin_write(worker.package.pins[memory.signals[SIGNAL_OE]], HIGH);
	}
}

void ic_dram_fill(const bool alternate = false) {
	uint8_t mask = (alternate && (memory.bus[BUS_D].width > 1)) ? memory.bus[BUS_D].high : 0;
	worker.color = COLOR_GOOD;
	for (memory.bus[BUS_CAS].address = 0; memory.bus[BUS_CAS].address < memory.bus[BUS_CAS].high; ++memory.bus[BUS_CAS].address) {
		worker.success = 0;
		worker.failure = 0;
		ic_bus_output(memory.bus[BUS_D]);
		// Inner loop is RAS to keep refreshing rows while writing a full column
		for (memory.bus[BUS_RAS].address = 0; memory.bus[BUS_RAS].address < memory.bus[BUS_RAS].high; ++memory.bus[BUS_RAS].address) {
			ic_dram_write();
			memory.bus[BUS_D].data ^= mask;
		}
		ic_bus_input(memory.bus[BUS_Q]);
		// Inner loop is RAS to keep refreshing rows while reading back a full column
		for (memory.bus[BUS_RAS].address = 0; memory.bus[BUS_RAS].address < memory.bus[BUS_RAS].high; ++memory.bus[BUS_RAS].address) {
			ic_dram_read();
			if (memory.bus[BUS_D].data != memory.bus[BUS_Q].data) {
#if MISMATCH
				Debug("RAS ");
				bin(memory.bus[BUS_RAS].address, memory.bus[BUS_RAS].width);
				Debug(", CAS ");
				bin(memory.bus[BUS_CAS].address, memory.bus[BUS_CAS].width);
				Debug(", D ");
				bin(memory.bus[BUS_D].data, memory.bus[BUS_D].width);
				Debug(", Q ");
				bin(memory.bus[BUS_Q].data, memory.bus[BUS_Q].width);
				Debug(", delta ");
				bin(memory.bus[BUS_D].data ^ memory.bus[BUS_Q].data, memory.bus[BUS_D].width);
				Debugln();
#endif
				worker.failure++;
				worker.color = COLOR_BAD;
			} else {
				worker.success++;
			}
			memory.bus[BUS_D].data ^= mask;
		}
		ui_draw_indicator((worker.success ? (worker.failure ? COLOR_MIXED : COLOR_GOOD) : COLOR_BAD));
	}
	ui_draw_ram();
}

//////////
// SRAM //
//////////

void ic_sram_idle() {
	// Idle
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_WE]], HIGH);
	if (ic_memory_signal(SIGNAL_OE)) {
		ic_pin_write(worker.package.pins[memory.signals[SIGNAL_OE]], HIGH);
	}
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_CS]], HIGH);
}

void ic_sram_write() {
#if 1
	// Address
	memory.write_address(memory.bus[BUS_ADDRESS]);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_CS]], LOW);
	// WE
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_WE]], LOW);
	// D
	ic_bus_write_data(memory.bus[BUS_D]);
	// Idle
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_WE]], HIGH);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_CS]], HIGH);
#else
	// Address
	memory.write_address(memory.bus[BUS_ADDRESS]);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_CS]], LOW);
	// Positive pulse
	bool pp = ic_memory_signal(SIGNAL_PP);
	if (pp) {
		ic_pin_write(worker.package.pins[memory.signals[SIGNAL_PP]], HIGH);
	}
	// D
	ic_bus_write_data(memory.bus[BUS_D]);
	// WE
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_WE]], LOW);
	// Idle
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_WE]], HIGH);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_CS]], HIGH);
	// Positive pulse
	if (pp) {
		ic_pin_write(worker.package.pins[memory.signals[SIGNAL_PP]], LOW);
	}
#endif
}

void ic_sram_read() {
#if 1
	// Address
	memory.write_address(memory.bus[BUS_ADDRESS]);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_CS]], LOW);
	// OE
	bool oe = ic_memory_signal(SIGNAL_OE);
	if (oe) {
		ic_pin_write(worker.package.pins[memory.signals[SIGNAL_OE]], LOW);
	}
	// Q
	ic_bus_read_data(memory.bus[BUS_Q]);
	// Idle
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_CS]], HIGH);
	// OE
	if (oe) {
		ic_pin_write(worker.package.pins[memory.signals[SIGNAL_OE]], HIGH);
	}
#else
	// Address
	memory.write_address(memory.bus[BUS_ADDRESS]);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_CS]], LOW);
	// OE
	bool oe = ic_memory_signal(SIGNAL_OE);
	if (oe) {
		ic_pin_write(worker.package.pins[memory.signals[SIGNAL_OE]], LOW);
	}
	// Positive pulse
	bool pp = ic_memory_signal(SIGNAL_PP);
	if (pp) {
		ic_pin_write(worker.package.pins[memory.signals[SIGNAL_PP]], HIGH);
	}
	// Q
	ic_bus_read_data(memory.bus[BUS_Q]);
	// Idle
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_CS]], HIGH);
	// OE
	if (oe) {
		ic_pin_write(worker.package.pins[memory.signals[SIGNAL_OE]], HIGH);
	}
	// Positive pulse
	if (pp) {
		ic_pin_write(worker.package.pins[memory.signals[SIGNAL_PP]], LOW);
	}
#endif
}

void ic_sram_fill(const bool alternate = false) {
	uint8_t mask = (alternate && (memory.bus[BUS_D].width > 1)) ? memory.bus[BUS_D].high : 0;
	worker.color = COLOR_GOOD;
	// Write full address space
	ic_bus_output(memory.bus[BUS_D]);
	for (memory.bus[BUS_ADDRESS].address = 0; memory.bus[BUS_ADDRESS].address < memory.bus[BUS_ADDRESS].high; ++memory.bus[BUS_ADDRESS].address) {
		ic_sram_write();
		memory.bus[BUS_D].data ^= mask;
	}
	// Read full address space
	ic_bus_input(memory.bus[BUS_Q]);
	for (memory.bus[BUS_ADDRESS].address = 0; memory.bus[BUS_ADDRESS].address < memory.bus[BUS_ADDRESS].high; ++memory.bus[BUS_ADDRESS].address) {
		worker.success = 0;
		worker.failure = 0;
		ic_sram_read();
		if (memory.bus[BUS_D].data != memory.bus[BUS_Q].data) {
#if MISMATCH
			Debug("A ");
			bin(memory.bus[BUS_ADDRESS].address, memory.bus[BUS_ADDRESS].width);
			Debug(", D ");
			bin(memory.bus[BUS_D].data, memory.bus[BUS_D].width);
			Debug(", Q ");
			bin(memory.bus[BUS_Q].data, memory.bus[BUS_Q].width);
			Debug(", delta ");
			bin(memory.bus[BUS_D].data ^ memory.bus[BUS_Q].data, memory.bus[BUS_D].width);
			Debugln();
#endif
			worker.failure++;
			worker.color = COLOR_BAD;
		} else {
			worker.success++;
		}
		memory.bus[BUS_D].data ^= mask;
		ui_draw_indicator((worker.success ? (worker.failure ? COLOR_MIXED : COLOR_GOOD) : COLOR_BAD));
	}
	ui_draw_ram();
}

//////////
// FRAM //
//////////

// FRAM
// - Erase.
// - Write from file.
void ic_fram_loop() {
	if (options[option_erase]) {
		memory.bus[BUS_D].data = 0xFF;
	}
	// File
	if (options[option_write]) {
		String filename(TILES_BROWSE.tile[board.tile()].label);
		file = fat.open(filename.c_str(), O_RDWR);
		ui_clear_subcontent();
		tft.print(F("Write "));
		tft.println(filename);
	}
	// UI
	uint16_t color = COLOR_OK;
	worker.color = COLOR_GOOD;
	ui_draw_rom();
	// Miscellaneous
	uint8_t data[16];
#if TIME
	// Timer
	unsigned long start = millis();
#endif
	// Setup
	for (uint8_t index = 0; index < memory.bus[BUS_D].width; ++index) {
		ic_pin_mode(worker.package.pins[memory.bus[BUS_D].pins[index]], INPUT);
	}
	// VCC, GND
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_GND]], LOW);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_VCC]], HIGH);
	// At package creation all pins are output low
	// LOW bus is preset, so only set HIGH bus high
	for (uint8_t index = 0; index < memory.bus[BUS_HIGH].width; ++index) {
		ic_pin_write(worker.package.pins[memory.bus[BUS_HIGH].pins[index]], HIGH);
	}
	// Idle
	memory.idle();
	// Write
	worker.indicator = 0;
	uint8_t index = 0;
	// Write full address space
	ic_bus_output(memory.bus[BUS_D]);
	for (memory.bus[BUS_ADDRESS].address = 0; memory.bus[BUS_ADDRESS].address < memory.bus[BUS_ADDRESS].high; ++memory.bus[BUS_ADDRESS].address) {
		if (options[option_write]) {
			if (index == 0) {
				file.read(data, 16);
			}
			memory.bus[BUS_D].data = data[index];
		}
		ic_sram_write();
		++index %= 16;
		ui_draw_indicator(color);
	}
	// Shutdown
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_VCC]], LOW);
#if TIME
	unsigned long stop = millis();
	Serial.print(F(TIME_ELAPSED));
	Serial.println((stop - start) / 1000.0);
#endif
	// File
	if (options[option_write]) {
		file.close();
	}
}

///////////
// Flash //
///////////

void ic_flash_id() {
	// Write
	ic_bus_output(memory.bus[BUS_D]);
	// 5555 AA
	// 2AAA 55
	// 5555 90
	memory.bus[BUS_ADDRESS].address = 0x5555;
	memory.bus[BUS_D].data = 0xAA;
	ic_sram_write();
	memory.bus[BUS_ADDRESS].address = 0x2AAA;
	memory.bus[BUS_D].data = 0x55;
	ic_sram_write();
	memory.bus[BUS_ADDRESS].address = 0x5555;
	memory.bus[BUS_D].data = 0x90;
	ic_sram_write();
	// Read
	ic_bus_input(memory.bus[BUS_Q]);
	// Manufacturer ID
	memory.bus[BUS_ADDRESS].address = 0x0000;
	ic_sram_read();
	uint8_t manufacturerID = memory.bus[BUS_Q].data;
	// Chip ID
	memory.bus[BUS_ADDRESS].address = 0x0001;
	ic_sram_read();
	uint8_t chipID = memory.bus[BUS_Q].data;
	// Write
	ic_bus_output(memory.bus[BUS_D]);
	// 5555 AA
	// 2AAA 55
	// 5555 F0
	memory.bus[BUS_ADDRESS].address = 0x5555;
	memory.bus[BUS_D].data = 0xAA;
	ic_sram_write();
	memory.bus[BUS_ADDRESS].address = 0x2AAA;
	memory.bus[BUS_D].data = 0x55;
	ic_sram_write();
	memory.bus[BUS_ADDRESS].address = 0x5555;
	memory.bus[BUS_D].data = 0xF0;
	ic_sram_write();
	// Output
	Debug("Manufacturer 0x");
	hexa(manufacturerID, 2);
	Debug(", chip 0x");
	hexa(chipID, 2);
	Debugln();
}

void ic_flash_erase() {
#if TIME
	// Timer
	unsigned long start = millis();
#endif
	// Write
	ic_bus_output(memory.bus[BUS_D]);
	// 5555 AA
	// 2AAA 55
	// 5555 80
	// 5555 AA
	// 2AAA 55
	// 5555 10
	memory.bus[BUS_ADDRESS].address = 0x5555;
	memory.bus[BUS_D].data = 0xAA;
	ic_sram_write();
	memory.bus[BUS_ADDRESS].address = 0x2AAA;
	memory.bus[BUS_D].data = 0x55;
	ic_sram_write();
	memory.bus[BUS_ADDRESS].address = 0x5555;
	memory.bus[BUS_D].data = 0x80;
	ic_sram_write();
	memory.bus[BUS_ADDRESS].address = 0x5555;
	memory.bus[BUS_D].data = 0xAA;
	ic_sram_write();
	memory.bus[BUS_ADDRESS].address = 0x2AAA;
	memory.bus[BUS_D].data = 0x55;
	ic_sram_write();
	memory.bus[BUS_ADDRESS].address = 0x5555;
	memory.bus[BUS_D].data = 0x10;
	ic_sram_write();
	// Delay TSCE, 100 ms
	delay(100);
	ic_toggle_bit(); // Not in SST39SF040.txt
#if TIME
	unsigned long stop = millis();
	Serial.print("Erase ");
	Serial.print((stop - start));
	Serial.println(" ms");
#endif
}

void ic_flash_write() {
	// Write
	ic_bus_output(memory.bus[BUS_D]);
	// 5555 AA
	// 2AAA 55
	// 5555 A0
	memory.bus[BUS_ADDRESS].address = 0x5555;
	memory.bus[BUS_D].data = 0xAA;
	ic_sram_write();
	memory.bus[BUS_ADDRESS].address = 0x2AAA;
	memory.bus[BUS_D].data = 0x55;
	ic_sram_write();
	memory.bus[BUS_ADDRESS].address = 0x5555;
	memory.bus[BUS_D].data = 0xA0;
	ic_sram_write();
}

void ic_toggle_bit() {
	uint8_t byte, same;
	// Read
	ic_bus_input(memory.bus[BUS_Q]);
	// DQ6 toggle
	do {
		ic_sram_read();
		byte = memory.bus[BUS_Q].data;
		ic_sram_read();
		same = memory.bus[BUS_Q].data;
	} while ((byte ^ same) & 0x40);
}

// Flash
// - Erase.
// - Write from file.
void ic_flash_loop() {
	// File
	if (options[option_write]) {
		String filename(TILES_BROWSE.tile[board.tile()].label);
		file = fat.open(filename.c_str(), O_RDWR);
		ui_clear_subcontent();
		tft.print(F("Write "));
		tft.println(filename);
	}
	// UI
	uint16_t color = COLOR_OK;
	worker.color = COLOR_GOOD;
	ui_draw_rom();
	// Miscellaneous
	uint8_t data[16];
#if TIME
	// Timer
	unsigned long start = millis();
#endif
	// Setup
	for (uint8_t index = 0; index < memory.bus[BUS_D].width; ++index) {
		ic_pin_mode(worker.package.pins[memory.bus[BUS_D].pins[index]], INPUT);
	}
	// VCC, GND
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_GND]], LOW);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_VCC]], HIGH);
	// At package creation all pins are output low
	// LOW bus is preset, so only set HIGH bus high
	for (uint8_t index = 0; index < memory.bus[BUS_HIGH].width; ++index) {
		ic_pin_write(worker.package.pins[memory.bus[BUS_HIGH].pins[index]], HIGH);
	}
	// Idle
	memory.idle();
	// Read ID
	ic_flash_id();
	// Erase
	ic_flash_erase();
	// Write
	if (options[option_write]) {
		worker.indicator = 0;
		uint8_t index = 0;
		for (uint32_t address = 0; address < memory.bus[BUS_ADDRESS].high; ++address) {
			if (index == 0) {
				file.read(data, 16);
				ui_draw_indicator(color);
			}
			ic_flash_write();
			memory.bus[BUS_ADDRESS].address = address;
			memory.bus[BUS_D].data = data[index];
			ic_sram_write();
			ic_toggle_bit();
			++index %= 16;
		}
	}
	// Shutdown
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_VCC]], LOW);
#if TIME
	unsigned long stop = millis();
	Serial.print(F(TIME_ELAPSED));
	Serial.println((stop - start) / 1000.0);
#endif
	// File
	if (options[option_write]) {
		file.close();
	}
}

/////////
// ROM //
/////////

// ROM, FRAM, Flash
// - Read to serial.
// - Read to file.
void ic_rom_loop() {
	if (options[option_file]) {
		String filename;
		sd_filename(filename, "output", ".bin");
		file = fat.open(filename.c_str(), O_CREAT | O_RDWR);
		ui_clear_subcontent();
		tft.print(F("Read "));
		tft.println(filename);
	}
	// UI
	uint16_t color = COLOR_OK;
	worker.color = COLOR_GOOD;
	ui_draw_rom();
	// Miscellaneous
	uint8_t count = (memory.bus[BUS_ADDRESS].width) / 4 + (memory.bus[BUS_ADDRESS].width % 4 ? 1 : 0);
	uint8_t data[16];
#if TIME
	// Timer
	unsigned long start = millis();
#endif
	// Setup
	for (uint8_t index = 0; index < memory.bus[BUS_Q].width; ++index) {
		ic_pin_mode(worker.package.pins[memory.bus[BUS_Q].pins[index]], INPUT);
	}
	// VCC, GND
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_GND]], LOW);
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_VCC]], HIGH);
	// At package creation all pins are output low
	// LOW bus is preset, so only set HIGH bus high
	for (uint8_t index = 0; index < memory.bus[BUS_HIGH].width; ++index) {
		ic_pin_write(worker.package.pins[memory.bus[BUS_HIGH].pins[index]], HIGH);
	}
	// Idle
	memory.idle();
	// Test
	worker.indicator = 0;
	uint8_t index = 0;
	bool blank = true;
	// Read full address space
	ic_bus_input(memory.bus[BUS_Q]);
	for (memory.bus[BUS_ADDRESS].address = 0; memory.bus[BUS_ADDRESS].address < memory.bus[BUS_ADDRESS].high; ++memory.bus[BUS_ADDRESS].address) {
		ic_sram_read();
		data[index] = memory.bus[BUS_Q].data;
		// Serial
		if (options[option_serial]) {
			if (index == 0) {
				hexa(memory.bus[BUS_ADDRESS].address, count);
			}
			if (index == 8) {
				Serial.print(" ");
			}
			Serial.print(" ");
			hexa(memory.bus[BUS_Q].data, 2);
		}
		// Blank
		if (blank && (memory.bus[BUS_Q].data != memory.bus[BUS_Q].high)) {
			blank = false;
			color = COLOR_KO;
			worker.color = COLOR_BAD;
			ui_draw_rom();
			if (options[option_blank]) {
				break;
			}
		}
		++index %= 16;
		if (index == 0) {
			// Serial
			if (options[option_serial]) {
				Serial.print(" ");
				for (uint8_t index = 0; index < 16; ++index) {
					if (index == 8) {
						Serial.print("  ");
					}
					if (iscntrl(char(data[index]))) {
						Serial.print('.');
					} else {
						Debug(char(data[index]));
					}
				}
				Serial.println();
			}
			// File
			if (options[option_file]) {
				file.write(data, 16);
			}
			ui_draw_indicator(color);
		}
	}
	// Shutdown
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_VCC]], LOW);
#if TIME
	unsigned long stop = millis();
	Serial.print(F(TIME_ELAPSED));
	Serial.println((stop - start) / 1000.0);
#endif
	// File
	if (options[option_file]) {
		file.close();
	}
}

void ic_rom_idle() {
	// Idle
	if (ic_memory_signal(SIGNAL_OE)) {
		ic_pin_write(worker.package.pins[memory.signals[SIGNAL_OE]], HIGH);
	}
	ic_pin_write(worker.package.pins[memory.signals[SIGNAL_CS]], HIGH);
}

//////////////////
// Touch screen //
//////////////////

#define TS_MIN 10
#define TS_MAX 1000
#define TS_DELAY 10

bool ts_touched() {
	TSPoint point = ts.getPoint();
	pinMode(YP, OUTPUT);
	pinMode(XM, OUTPUT);
	digitalWrite(YP, HIGH);
	digitalWrite(XM, HIGH);
	if (point.z != 0) {
		// TODO adapt code to your display
		worker.x = map(point.y, TS_LEFT, TS_RT, TFT_WIDTH, 0);
		worker.y = map(point.x, TS_TOP, TS_BOT, 0, TFT_HEIGHT);
		if ((point.z > TS_MIN) && (point.z < TS_MAX)) {
			return true;
		}
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
	pinMode(YP, OUTPUT);
	pinMode(XM, OUTPUT);
	digitalWrite(YP, HIGH);
	digitalWrite(XM, HIGH);
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

bool sd_begin() {
#if SD_FAT_VERSION < 20000
	return fat.begin(10);
#else
	return fat.begin(SDCONFIG);
#endif
}

void sd_filename(String& filename, const char* section, const char* extension) {
	char buffer[5];
	uint32_t index = ini_read_key(section, "index");
	ini_write_key(section, "index", ++index);
	filename ="ict-";
	snprintf(buffer, 5, "%04ld", index);
	filename += buffer;
	filename += extension;
}

#if CAPTURE
void sd_screen_capture() {
	String filename;
	sd_filename(filename, "capture", ".data");
	Serial.print("Save ");
	Serial.print(filename);
	uint16_t* pixels = (uint16_t*)malloc(sizeof(uint16_t) * TFT_WIDTH);
	if (pixels) {
		File file;
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

bool compare(Filename*& a, Filename*& b) {
	return String((*a).Buffer()) > String((*b).Buffer());
}

#define FILENAME_SIZE 256

void sd_root() {
	fat.chdir();
}

void sd_browse() {
	char buffer[FILENAME_SIZE];
	files.clear();
	folders.clear();
	// Build the list
	File root = fat.open(path);
	root.rewind();
	// Loop files
	while (file.openNext(&root, O_RDONLY)) {
		file.getName(buffer, FILENAME_SIZE);
		if (!file.isHidden()) {
			Filename* filename = new Filename(buffer);
			if (file.isDir()) {
				folders.push(filename);
			} else {
				files.push(filename);
			}
		}
		file.close();
	}
	root.close();
	// Sort, folders then files, alphabetical order
	folders.sort(compare);
	files.sort(compare);
	worker.first = 0;
	worker.last = folders.count() + files.count();
	worker.last = ((worker.last + 3) / 4 - 1) * 4;
	sd_display();
}

void sd_folder(uint16_t tile) {
	uint16_t index = worker.first + tile;
	if (index < folders.count()) {
		if (path.equals("/")) {
			path = folders[index]->Buffer();
		} else {
			path += "/";
			path += folders[index]->Buffer();
		}
		sd_browse();
	}
}

void sd_display() {
	for (uint8_t tile = 0; tile < 4; ++tile) {
		uint16_t index = worker.first + tile;
		if (index < folders.count()) {
			TILES_BROWSE.tile[tile].label = folders[index]->Buffer();
			TILES_BROWSE.tile[tile].flags = TILE_FLAG_FOLDER;
		} else {
			index -= folders.count();
			if (index < files.count()) {
				TILES_BROWSE.tile[tile].label = files[index]->Buffer();
			TILES_BROWSE.tile[tile].flags = TILE_FLAG_FILE;
			} else {
				TILES_BROWSE.tile[tile].label = NULL;
				TILES_BROWSE.tile[tile].flags = TILE_FLAG_NONE;
			}
		}
	}
	board.draw();
}

//////////////
// INI file //
//////////////

uint32_t ini_read_key(const char* section, const char* key) {
	file = fat.open(INIFILE);
	if (file) {
		bool in = false;
		// Patterns
		String patterns[2];
		patterns[0] = "[";
		patterns[0].concat(section);
		patterns[0].concat("]");
		patterns[1].concat(key);
		patterns[1].concat("=");
		while (file.available()) {
			String line = file.readStringUntil('\n');
			line.trim();
			// Key
			if (in && line.startsWith(patterns[1])) {
				String value = line.substring(line.indexOf("=") + 1);
				value.toUpperCase();
				if (value.startsWith ("0X")) {
					return strtoul(value.c_str(), NULL, 16);
				} else {
					return value.toInt();
				}
				break;
			} else {
				// Section
				if (line.startsWith("[")) {
					in = line.equalsIgnoreCase(patterns[0]);
				}
			}
		}
		file.close();
	} else {
		Debug(INIFILE);
		Debugln(F(" not found"));
		worker.task = task_media;
	}
	return 0;
}

void ini_write_key(const char* section, const char* key, uint32_t value) {
	File input = fat.open(INIFILE);
	File output = fat.open(TMPFILE, O_CREAT | O_RDWR);
	if (input && output) {
		bool in = false;
		// Patterns
		String patterns[2];
		patterns[0] = "[";
		patterns[0].concat(section);
		patterns[0].concat("]");
		patterns[1].concat(key);
		patterns[1].concat("=");
		while (input.available()) {
			String line = input.readStringUntil('\n');
			line.trim();
			// Key
			if (in && line.startsWith(patterns[1])) {
				output.print(key);
				output.print("=");
				output.println(value);
			} else {
				// Section
				if (line.startsWith("[")) {
					in = line.equalsIgnoreCase(patterns[0]);
				}
				output.println(line);
			}
		}
		input.close();
		fat.remove(INIFILE);
		output.rename(INIFILE);
		output.close();
	} else {
		Debug(INIFILE);
		Debugln(F(" not found"));
		worker.task = task_media;
	}
}

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

void ui_draw_wrap(const String& string, const uint8_t size) {
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

void ui_draw_indicator(const uint16_t color) {
	tft.fillRect(worker.indicator * 20 + 2, 222, 16, 16, color);
	++worker.indicator %= 16;
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
	switch (worker.ram++) {
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
}

void ui_draw_rom() {
	unsigned int x = (TFT_WIDTH - RAM_BOX)/ 2;
	tft.fillRect(x, RAM_TOP, RAM_SIZE, RAM_SIZE, worker.color);
	tft.setTextSize(3);
	tft.setTextColor(TFT_BLACK);
	ui_draw_center(F("FF"), RAM_TOP + (RAM_BOX - 3 * 8) / 2);
}

void ui_draw_header(bool erase) {
	if (erase) {
		tft.fillScreen(COLOR_BACKGROUND);
	}
	tft.setTextColor(COLOR_TEXT);
	tft.setTextSize(5);
	ui_draw_center(F("ICT"), 2);
//	Serial.print("HEADER BOTTOM ");
//	Serial.println(tft.getCursorY());
}

void ui_draw_subheader(const __FlashStringHelper* text, bool erase) {
	if (erase) {
		ui_clear_subheader();
	}
	tft.setTextColor(COLOR_TEXT);
	tft.setTextSize(2);
	ui_draw_center(text, 42);
//	Serial.print("SUB HEADER BOTTOM ");
//	Serial.println(tft.getCursorY());
}

void ui_draw_button(char* label) {
	// ENTER, CONTINUE, TEST or REDO, always button 0 and same shape and color
	buttons[0].initButton(&tft, BUTTON_X + (BUTTON_W + BUTTON_SPACING_X) * 0.5,
			BUTTON_Y, 2 * BUTTON_W + BUTTON_SPACING_X, BUTTON_H,
			COLOR_ENTER, COLOR_ENTER, COLOR_LABEL, label, BUTTON_TEXTSIZE);
	buttons[0].drawButton();
}

void ui_draw_escape() {
	buttons[button_escape].initButton(&tft, BUTTON_X + (BUTTON_W + BUTTON_SPACING_X) * 4,
			BUTTON_Y, BUTTON_W, BUTTON_H,
			COLOR_ESCAPE, COLOR_ESCAPE, COLOR_LABEL, (char*)"ESC", BUTTON_TEXTSIZE);
	buttons[button_escape].drawButton();
}

void ui_draw_screen() {
	switch (worker.screen) {
	case screen_startup:
		ui_draw_header(true);
		// Version
		tft.setTextSize(2);
		ui_draw_center(F(ICT_VERSION), tft.getCursorY());
		ui_draw_center(F(ICT_DATE), tft.getCursorY());
		// Author
		tft.setTextColor(TFT_WHITE);
		ui_draw_center(F("Patrick Lafarguette"), AREA_CONTENT + 32);
		ui_draw_escape();
		break;
	case screen_media:
		ui_draw_header(true);
		// Error
		ui_draw_error(F(INSERT_SD_CARD));
		ui_draw_escape();
		break;
	case screen_menu:
		worker.action = action_idle;
		ui_draw_header(true);
		ui_draw_subheader(F(MAIN_MENU), false);
		board.tiles(TILES_MENU);
		board.size(3);
		board.draw();
		break;
	case screen_logic:
		switch (worker.action) {
		case action_menu:
			ui_draw_header(true);
			ui_draw_subheader(F(MAIN_MENU), false);
			board.tiles(TILES_LOGIC);
			board.size(2);
			board.draw();
			ui_draw_escape();
			break;
		case action_identify:
			worker.index = -1;
			ics.clear();
			ui_draw_header(true);
			ui_draw_subheader(F(IDENTIFY_LOGIC), false);
			ic_package_create();
			ic_logic_identify();
			break;
		case action_identified:
			ui_clear_footer();
			if (ics.count()) {
				ui_draw_button((char*)"TEST");
				if (ics.count() > 1) {
					// Add navigator
					buttons[button_previous].initButton(&tft, BUTTON_X + 2 * (BUTTON_W + BUTTON_SPACING_X),
							BUTTON_Y, BUTTON_W, BUTTON_H,
							COLOR_KEY, COLOR_KEY, COLOR_LABEL, (char*)"<", BUTTON_TEXTSIZE);
					buttons[button_previous].drawButton();
					buttons[button_next].initButton(&tft, BUTTON_X + 3 * (BUTTON_W + BUTTON_SPACING_X),
							BUTTON_Y,  BUTTON_W, BUTTON_H,
							COLOR_KEY, COLOR_KEY, COLOR_LABEL, (char*)">", BUTTON_TEXTSIZE);
					buttons[button_next].drawButton();
				}
			} else {
				ui_draw_content();
			}
			ui_draw_escape();
			break;
		case action_load:
			ics.clear();
			lines.clear();
			ui_draw_header(true);
			ui_draw_subheader(F(LOAD_LOGIC), false);
			if (ic_load(LOGICFILE)) {
				ui_draw_subheader(F(LOGIC_FOUND), true);
				ui_clear_footer();
				ui_draw_button((char*)"TEST");
				ui_draw_escape();
			} else {
				ui_draw_error(F(NO_MATCH_FOUND));
				ui_clear_footer();
				ui_draw_escape();
			}
			break;
		case action_reload:
			ics.clear();
			lines.clear();
			ui_draw_header(true);
			ui_draw_subheader(F(LOAD_LOGIC), true);
			if (ic_load(LOGICFILE)) {
				worker.action = action_test;
				ui_draw_screen();
			} else {
				ui_draw_error(F(NO_MATCH_FOUND));
				ui_clear_footer();
				ui_draw_escape();
			}
			break;
		case action_test:
			ui_draw_subheader(F(LOGIC_TEST), true);
			ui_clear_subcontent();
			ui_clear_footer();
			ic_logic_test();
			unsigned int total = worker.success + worker.failure;
			double percent = (100.0 * worker.success) / total;
			tft.print(total);
			tft.print(F(CYCLES));
			tft.print(percent);
			tft.println(F(PASSED));
			tft.setTextSize(2);
			tft.setTextColor(worker.success ? (worker.failure ? COLOR_MIXED : COLOR_GOOD) : COLOR_BAD);
			tft.println(worker.success ? (worker.failure ? F(UNRELIABLE) : F(GOOD)) : F(BAD));
			ui_clear_footer();
			ui_draw_button((char*)"REDO");
			ui_draw_escape();
			worker.action = action_redo; // Only to trigger test
			break;
		}
		break;
	case screen_keyboard:
		tft.fillScreen(TFT_NAVY);
		tft.drawRect(0, 0, TFT_WIDTH, INPUT_HEIGHT, TFT_YELLOW);
		tft.drawRect(1, 1, TFT_WIDTH - 2, INPUT_HEIGHT - 2, TFT_YELLOW);
		keyboard.draw();
		ui_draw_button((char*)"ENTER");
		buttons[button_clear].initButton(&tft, BUTTON_X + (BUTTON_W + BUTTON_SPACING_X) * 2.5,
				BUTTON_Y, 2 * BUTTON_W + BUTTON_SPACING_X, BUTTON_H,
				COLOR_KEY, COLOR_KEY, COLOR_LABEL, (char*)"CLR", BUTTON_TEXTSIZE);
		buttons[button_clear].drawButton();
		buttons[button_del].initButton(&tft, BUTTON_X + (BUTTON_W + BUTTON_SPACING_X) * 4,
				BUTTON_Y, BUTTON_W, BUTTON_H,
				COLOR_DELETE, COLOR_DELETE, COLOR_LABEL, (char*)"DEL", BUTTON_TEXTSIZE);
		buttons[button_del].drawButton();
		// TODO only here?
		worker.code = "";
		worker.position = 0;
		break;
	case screen_memory:
		switch (worker.action) {
		case action_load:
			ics.clear();
			lines.clear();
			ui_draw_header(true);
			ui_draw_subheader(F(LOAD_MEMORY), false);
			if (ic_load(MEMORYFILE)) {
				ic_parse_memory();
				ic_setup_memory();
				ui_draw_subheader(F(MEMORY_FOUND), true);
				ui_clear_footer();
				ui_draw_button((char*)"CONTINUE");
				ui_draw_escape();
			} else {
				ui_draw_error(F(NO_MATCH_FOUND));
				ui_clear_footer();
				ui_draw_escape();
			}
			break;
		case action_option:
			ui_clear_content();
			ui_draw_content();
			worker.action = action_test;
			ui_draw_screen();
			break;
		case action_test:
			ui_draw_subheader(F(MEMORY_TEST), true);
			ui_clear_footer();
			ic_memory_test();
			ui_clear_footer();
			ui_draw_button((char*)"REDO");
			ui_draw_escape();
			worker.action = action_redo; // Only to trigger test
			break;
		}
		break;
	case screen_option:
		worker.action = action_idle;
		ui_draw_header(true);
		switch (ic.type) {
		case TYPE_FRAM:
		case TYPE_FLASH:
			ui_draw_subheader(F(FRAM_OPTION), false);
			board.tiles(TILES_FRAM);
			board.size(2);
			break;
		case TYPE_ROM:
			ui_draw_subheader(F(ROM_OPTION), false);
			board.tiles(TILES_ROM);
			board.size(2);
			break;
		}
		board.draw();
		ui_draw_button((char*)"CONTINUE");
		ui_draw_escape();
		break;
	case screen_browse:
		switch (worker.action) {
		case action_browse:
			ui_draw_header(true);
			ui_draw_subheader(F(WRITE_FILE), false);
			path = "/";
			board.tiles(TILES_BROWSE);
			board.size(2);
			sd_browse();
			buttons[button_root].initButton(&tft, BUTTON_X + 0 * (BUTTON_W + BUTTON_SPACING_X),
					BUTTON_Y, BUTTON_W, BUTTON_H,
					COLOR_KEY, COLOR_KEY, COLOR_LABEL, (char*)"/", BUTTON_TEXTSIZE);
			buttons[button_root].drawButton();
			buttons[button_up].initButton(&tft, BUTTON_X + 1 * (BUTTON_W + BUTTON_SPACING_X),
					BUTTON_Y,  BUTTON_W, BUTTON_H,
					COLOR_KEY, COLOR_KEY, COLOR_LABEL, (char*)"^", BUTTON_TEXTSIZE);
			buttons[button_up].drawButton();
			buttons[button_previous].initButton(&tft, BUTTON_X + 2 * (BUTTON_W + BUTTON_SPACING_X),
					BUTTON_Y, BUTTON_W, BUTTON_H,
					COLOR_KEY, COLOR_KEY, COLOR_LABEL, (char*)"<", BUTTON_TEXTSIZE);
			buttons[button_previous].drawButton();
			buttons[button_next].initButton(&tft, BUTTON_X + 3 * (BUTTON_W + BUTTON_SPACING_X),
					BUTTON_Y,  BUTTON_W, BUTTON_H,
					COLOR_KEY, COLOR_KEY, COLOR_LABEL, (char*)">", BUTTON_TEXTSIZE);
			buttons[button_next].drawButton();
			ui_draw_escape();
			break;
		}
		break;
	}
}

void ui_clear_header() {
	tft.fillRect(0, 0, TFT_WIDTH, AREA_CONTENT, COLOR_BACKGROUND);
}

void ui_clear_subheader() {
	tft.fillRect(0, 42, TFT_WIDTH, AREA_CONTENT - 42, COLOR_BACKGROUND);
}

void ui_clear_content() {
	tft.fillRect(0, AREA_CONTENT, TFT_WIDTH, AREA_FOOTER - AREA_CONTENT, COLOR_BACKGROUND);
	tft.setCursor(0, AREA_CONTENT);
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(2);
}

void ui_clear_subcontent() {
	tft.fillRect(0, worker.content, TFT_WIDTH, AREA_FOOTER - worker.content, COLOR_BACKGROUND);
	tft.setCursor(0, worker.content);
	tft.setTextColor(TFT_WHITE);
	tft.setTextSize(2);
}

void ui_clear_footer() {
	tft.fillRect(0, AREA_FOOTER, TFT_WIDTH, TFT_HEIGHT - AREA_FOOTER, COLOR_BACKGROUND);
}

void ui_draw_content() {
	// IC counter
	switch (worker.action) {
	case action_identify:
	case action_identified:
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
	}
	// IC description
	tft.setCursor(0, tft.getCursorY() + 4);
	tft.setTextColor(COLOR_CODE);
	tft.setTextSize(3);
	tft.println(ics[worker.index]->code);
	tft.setCursor(0, tft.getCursorY() + 4);
	tft.setTextColor(COLOR_DESCRIPTION);
	ui_draw_wrap(ics[worker.index]->description, 2);
	worker.content = tft.getCursorY();
}

void ui_draw_ic(int count, bool wide) {
	tft.fillScreen(COLOR_BACKGROUND);
	int width = (count / 2) * 12 + 6;
	int height = wide ? 80 : 40;
	int left = (tft.width() - width) / 2;
	int top = (tft.height() - height) / 2;
	int bottom = top + height;
	// Body
	tft.fillRect(left, top, width, height, TFT_BLACK);
	tft.fillCircleHelper(left + width, tft.height() / 2, 6, 3, 0, COLOR_BACKGROUND);
	// Pins
	int x = left + 6;
	for (int index = 0; index < (count / 2); ++index) {
		tft.fillRect(x, top - 6, 6, 6, TFT_DARKGREY);
		tft.fillRect(x, bottom, 6, 6, TFT_DARKGREY);
		x += 12;
	}
}

void ui_draw_pin(int count, bool wide, int pin, uint16_t color) {
	int width = (count / 2) * 12 + 6;
	int height = wide ? 80 : 40;
	int left = (tft.width() - width) / 2;
	int top = (tft.height() - height) / 2;
	int bottom = top + height;
	// Pins
	int x = left + 6;
	if (pin < count / 2) {
		x += ((count / 2) - pin % (count / 2) - 1) * 12;
	} else {
		x += (pin - (count / 2)) * 12;
	}
	int y = pin < (count / 2) ? top - 6 : bottom;
	tft.fillRect(x, y, 6, 6, color);
}

bool ux_button_pressed(uint8_t index, bool touched) {
	if (touched && buttons[index].contains(worker.x, worker.y)) {
		buttons[index].press(true);
	} else {
		buttons[index].press(false);
	}
	if (buttons[index].justReleased()) {
		buttons[index].drawButton();
	}
	if (buttons[index].justPressed()) {
		buttons[index].drawButton(true);
		return true;
	}
	return false;
}

////////////////////////
// Integrated circuit //
////////////////////////

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
	for (uint8_t index = 0; index < worker.package.count; ++index) {
		ic_pin_mode(worker.package.pins[index], OUTPUT);
		ic_pin_write(worker.package.pins[index], LOW);
	}
}

void ic_package_idle() {
	for (uint8_t index = 0; index < worker.package.count; ++index) {
		ic_pin_mode(worker.package.pins[index], INPUT);
	}
}

#if TEST
void ic_package_dump() {
	Serial.print("DIP");
	Serial.print(worker.package.width);
	Serial.print(" ");
	for (uint8_t index = 0; index < worker.package.width; ++index) {
		if (index) {
			Serial.print(", ");
		}
		// TODO adapt to Pin
		// Serial.print(worker.package.pins[index]);
	}
	Serial.println();
}

void ic_package_test() {
	worker.package.width = 20;
	ic_package_create();
	// 30, 32, 34, 36, 38, 40, 42, 44, 50, 48, 49, 47, 45, 43, 41, 39, 37, 35, 33, 31
	ic_package_dump();
	worker.package.width = 18;
	ic_package_create();
	// 30, 32, 34, 36, 38, 40, 42, 44, 50, 47, 45, 43, 41, 39, 37, 35, 33, 31
	ic_package_dump();
	worker.package.width = 16;
	ic_package_create();
	// 30, 32, 34, 36, 38, 40, 42, 44, 45, 43, 41, 39, 37, 35, 33, 31
	// 30, 32, 34, 36, 38, 40, 42, 44, 45, 43, 41, 39, 37, 35, 33, 31
	ic_package_dump();
	worker.package.width = 14;
	ic_package_create();
	// 30, 32, 34, 36, 38, 40, 42, 43, 41, 39, 37, 35, 33, 31
	// 30, 32, 34, 36, 38, 40, 42, 43, 41, 39, 37, 35, 33, 31
	ic_package_dump();
}
#endif

bool ic_logic_test(String line) {
	bool result = true;
	bool clock = false;
#if 0
	Debugln(line);
#endif
	// VCC, GND and input
	for (uint8_t index = 0; index < worker.package.count; ++index) {
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
	for (uint8_t index = 0; index < worker.package.count; ++index) {
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
			clock = true;
			ic_pin_mode(worker.package.pins[index], OUTPUT);
			ic_pin_write(worker.package.pins[index], LOW);
			break;
		}
	}
	// Clock
	if (clock) {
		for (uint8_t index = 0; index < worker.package.count; ++index) {
			if (line[index] == 'C') {
				ic_pin_write(worker.package.pins[index], HIGH);
			}
		}
		delay(10);
		for (uint8_t index = 0; index < worker.package.count; ++index) {
			if (line[index] == 'C') {
				ic_pin_write(worker.package.pins[index], LOW);
			}
		}
	}
	delay(5);
	// Read
	for (uint8_t index = 0; index < worker.package.count; ++index) {
		switch (line[index]) {
		case 'H':
			if (!ic_pin_read(worker.package.pins[index])) {
				result = false;
#if 0
				Debug(F(LOGIC_LOW));
#endif
			} else {
#if 0
				Debug(F(LOGIC_SPACE));
#endif
			}
			break;
		case 'L':
			if (ic_pin_read(worker.package.pins[index])) {
				result = false;
#if 0
				Debug(F(LOGIC_HIGH));
#endif
			} else {
#if 0
				Debug(F(LOGIC_SPACE));
#endif
			}
			break;
#if 0
		default:
			Debug(F(LOGIC_SPACE));
#endif
		}
	}
#if 0
	Debugln();
#endif
	return result;
}

void ic_logic_identify() {
	String line;
	file = fat.open(LOGICFILE);
	if (file) {
		worker.indicator= 0;
		while (file.available()) {
			file.readStringUntil('$');
			if (!file.available()) {
				// Avoid timeout at the end of file
				break;
			}
			ic.position = file.position() - 1;
			ic.code = file.readStringUntil('\n');
			ic.code.trim();
			ic.description = file.readStringUntil('\n');
			ic.count = file.readStringUntil('\n').toInt();
			if (worker.package.count == ic.count) {
				worker.ok = true;
				while (file.peek() != '$') {
					line = file.readStringUntil('\n');
					line.trim();
					if (ic_logic_test(line) == false) {
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
		}
		file.close();
		worker.action = action_identified;
	} else {
		Debug(LOGICFILE);
		Debugln(F(" not found"));
		worker.task = task_media;
	}
	ui_draw_screen();
}

bool ic_load(const char* filename) {
	worker.found = false;
	// TODO move to root elsewhere?
	sd_root();
	file = fat.open(filename);
	if (file) {
		worker.indicator = 0;
		file.seek(worker.position);
		while (file.available()) {
			file.readStringUntil('$');
			if (!file.available()) {
				// Avoid timeout at the end of file
				break;
			}
			// Aliases
			bool first = true;
			bool loop = true;
			String line;
			while (loop) {
				line = file.readStringUntil('\n');
				line.trim();
				if (line.startsWith("//")) {
					// It is a Comment, skip line
					Debug("Skip ");
					Debugln(line);
					continue;
				}
				if (line[0] == '$' || first) {
					// Code and aliases
					if (worker.found == false) {
						String code;
						int start = first ? 0 : 1;
						int stop = 0;
						first = false;
						while ((stop = line.indexOf(':', start)) > -1) {
							code = line.substring(start, stop++);
							if (worker.code.equalsIgnoreCase(code)) {
								ic.code = code;
								worker.found = true;
							}
							start = stop;
						}
						// Last alias
						code = line.substring(start);
						if (worker.code.equalsIgnoreCase(code)) {
							ic.code = code;
							worker.found = true;
						}
					}
				} else {
					ic.description = line;
					loop = false;
				}
			}
			ic.count = file.readStringUntil('\n').toInt();
			if (worker.found) {
				worker.index = 0;
				worker.package.count = ic.count;
				ic_package_create();
				ic_package_output();
				ics.push(new IC(ic));
				ui_draw_content();
				while (file.peek() != '$') {
					line = file.readStringUntil('\n');
					line.trim();
					if (line.startsWith("//")) {
						// It is a Comment, skip line
						Debug("Skip ");
						Debugln(line);
						continue;
					}
					lines.push(new String(line));
				}
				break;
			} else {
				ui_draw_indicator(COLOR_SKIP);
			}
		}
		file.close();
	} else {
		Debug(filename);
		Debugln(F(" not found"));
		worker.task = task_media;
	}
	return worker.found;
}
void ic_logic_test() {
	worker.indicator = 0;
	worker.success = 0;
	worker.failure = 0;
	bool loop = true;
	while (loop) {
		worker.ok = true;
		for (uint8_t index = 0; index < lines.count(); ++index) {
			if (ic_logic_test(*lines[index]) == false) {
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
}

bool ic_parse_bus(String& tag, String& data) {
	const char* LABELS[] = { "A", "R", "C", "D", "Q", "L", "H" };
	const uint8_t BUS[] = { BUS_ADDRESS, BUS_RAS, BUS_CAS, BUS_D, BUS_Q, BUS_LOW, BUS_HIGH };
	for (uint8_t index = 0; index < sizeof(BUS) / sizeof(uint8_t); ++index) {
		if (tag.equals(LABELS[index])) {
			int start = 0, stop = 0;
			uint8_t pin = 0;
			while ((stop = data.indexOf(' ', start)) > -1) {
				pin = data.substring(start, stop++).toInt();
				Debug(", ");
				Debug(pin);
				memory.bus[BUS[index]].pins[memory.bus[BUS[index]].width++] = pin - 1;
				start = stop;
			}
			pin = data.substring(start).toInt();
			Debug(", ");
			Debug(pin);
			memory.bus[BUS[index]].pins[memory.bus[BUS[index]].width++] = pin - 1;
			memory.bus[BUS[index]].high = (uint32_t)1 << memory.bus[BUS[index]].width;
			Debug(", width ");
			Debug(memory.bus[BUS[index]].width);
			Debug(", high 0x");
			Debug(memory.bus[BUS[index]].high, HEX);
			return true;
		}
	}
	return false;
}

bool ic_parse_signal(String& tag, String& data) {
	const char *LABELS[] = { "CS", "RAS", "CAS", "WE", "OE", "PP", "NP", "GND",
			"VCC", "VBB", "VDD" };
	const uint8_t SIGNALS[] = { SIGNAL_CS, SIGNAL_RAS, SIGNAL_CAS, SIGNAL_WE,
			SIGNAL_OE, SIGNAL_PP, SIGNAL_NP, SIGNAL_GND, SIGNAL_VCC, SIGNAL_VBB,
			SIGNAL_VDD };
	for (uint8_t index = 0; index < sizeof(SIGNALS) / sizeof(uint8_t); ++index) {
		if (tag.equals(LABELS[index])) {
			uint8_t pin = data.toInt();
			Debug(F(" "));
			Debug(pin);
			memory.signals[SIGNALS[index]] = pin - 1;
			return true;
		}
	}
	return false;
}

void ic_parse_memory() {
	memory.count = ic.count;
	// Reset
	memory.bus[BUS_RAS].width = 0;
	memory.bus[BUS_CAS].width = 0;
	memory.bus[BUS_D].width = 0;
	memory.bus[BUS_Q].width = 0;
	memory.bus[BUS_LOW].width = 0;
	memory.bus[BUS_HIGH].width = 0;
	for (uint8_t index = 0; index < SIGNAL_COUNT; ++index) {
		memory.signals[index] = -1;
	}
	if (lines[0]->equals("DRAM")) {
		ic.type = TYPE_DRAM;
		Debugln("DRAM");
	}
	if (lines[0]->equals("SRAM")) {
		ic.type = TYPE_SRAM;
		Debugln("SRAM");
	}
	if (lines[0]->equals("FRAM")) {
		ic.type = TYPE_FRAM;
		Debugln("FRAM");
	}
	if (lines[0]->equals("FLASH")) {
		ic.type = TYPE_FLASH;
		Debugln("FLASH");
	}
	if (lines[0]->equals("ROM")) {
		ic.type = TYPE_ROM;
		Debugln("ROM");
	}
	// Parse
	for (uint8_t index = 1; index < lines.count(); ++index) {
		int stop = lines[index]->indexOf(' ');
		String* line = lines[index];
		String tag = line->substring(0, stop);
		String data = line->substring(stop + 1);
		Debug("tag [");
		Debug(tag);
		if (ic_parse_signal(tag, data) || ic_parse_bus(tag, data)) {
			Debugln("]");
		} else {
			Serial.println("] unknown");
		}
	}
	// Decrease data bus
	memory.bus[BUS_D].high -= 1;
	memory.bus[BUS_Q].high -= 1;
}

void ic_setup_memory() {
	switch (ic.type) {
	case TYPE_DRAM:
		// Read/write test
		memory.loop = &ic_ram_loop;
		memory.idle = &ic_dram_idle;
		memory.fill = &ic_dram_fill;
		if ((memory.bus[BUS_RAS].width > 16) || (memory.bus[BUS_CAS].width > 16)) {
			memory.write_address = &ic_bus_write_address_dword;
		} else {
			memory.write_address = &ic_bus_write_address_word;
		}
		break;
	case TYPE_SRAM:
	case TYPE_FRAM:
	case TYPE_FLASH:
		// Read/write test
		// TODO default read/write not suitable for Flash
		memory.loop = &ic_ram_loop;
		memory.idle = &ic_sram_idle;
		memory.fill = &ic_sram_fill;
		if (memory.bus[BUS_ADDRESS].width > 16) {
			memory.write_address = &ic_bus_write_address_dword;
		} else {
			memory.write_address = &ic_bus_write_address_word;
		}
		break;
	case TYPE_ROM:
		// Read, blank
		memory.loop = &ic_rom_loop;
		memory.idle = &ic_rom_idle;
		if (memory.bus[BUS_ADDRESS].width > 16) {
			memory.write_address = &ic_bus_write_address_dword;
		} else {
			memory.write_address = &ic_bus_write_address_word;
		}
		break;
	}
}

void ic_memory_test() {
	memory.loop();
}

///////////////////////////////
// Interrupt Service Routine //
///////////////////////////////

void isr_user_button() {
	worker.interrupted = true;
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
	// Power rails default to TTL
	digitalWrite(A7, LOW);
	pinMode(A7, OUTPUT);
	// User button
	pinMode(21, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(21), isr_user_button, LOW);
	// TFT setup
	tft_init();
#if 0
	ui_draw_ic(20, false);
	ui_draw_ic(40, true);
	// 4116
	ui_draw_ic(16, false);
	// VBB -5 V white
	// VSS GND black
	// VCC +5 V red
	// VDD +12 V yellow
	ui_draw_pin(16, false, 0, TFT_WHITE);
	ui_draw_pin(16, false, 7, TFT_YELLOW);
	ui_draw_pin(16, false, 8, TFT_RED);
	ui_draw_pin(16, false, 15, TFT_BLACK);
	while (true) {
		ts_wait();
		Serial.println("12V, 5V");
		digitalWrite(A7, HIGH);
		delay(1000);
		ts_wait();
		Serial.println("TTL");
		digitalWrite(A7, LOW);
		delay(1000);
	};
#endif
	// SD card
	worker.task = sd_begin() ? task_startup : task_media;
	// UI
	ui_draw_screen();
}

void loop() {
	bool touched = ts_touched();
	uint8_t task = worker.task;
	uint8_t screen = worker.screen;
	uint8_t action = worker.action;

	////////////////////
	// Change screen? //
	////////////////////

	if (touched) {
		switch (worker.screen) {
		case screen_startup:
			if (buttons[button_escape].contains(worker.x, worker.y)) {
				task = task_menu;
				screen = screen_menu;
			}
			break;
		case screen_menu:
			if (board.input(worker.x, worker.y)) {
				switch (board.tile()) {
				case tile_logic:
					task = task_logic;
					screen = screen_logic;
					action = action_menu;
					break;
				case tile_memory:
					task = task_memory;
					screen = screen_keyboard;
					action = action_keyboard;
					break;
				}
			}
			break;
		case screen_logic:
			switch (worker.action) {
				case action_identified:
					if (buttons[button_test].contains(worker.x, worker.y)) {
						worker.code = ics[worker.index]->code;
						worker.position = ics[worker.index]->position;
						action = action_reload;
					}
					break;
				case action_menu:
					if (board.input(worker.x, worker.y)) {
						switch (board.tile()) {
						case tile_keyboard:
							screen = screen_keyboard;
							action = action_keyboard;
							break;
						default:
							worker.package.count = PIN_COUNT[board.tile()];
							task = task_logic;
							action = action_identify;
							break;
						}
					}
					break;
			}
			// TEST or REDO
			if (buttons[button_continue].contains(worker.x, worker.y)) {
				switch (worker.action) {
				case action_load:
				case action_redo:
					action = action_test;
					break;
				}
			}
			// ESCape to menu
			if (buttons[button_escape].contains(worker.x, worker.y)) {
				task = task_menu;
				screen = screen_menu;
			}
			break;
		case screen_memory:
			// CONTINUE or REDO
			if (buttons[button_continue].contains(worker.x, worker.y)) {
				switch (worker.action) {
				case action_load:
					switch (ic.type) {
					case TYPE_FRAM:
					case TYPE_FLASH:
					case TYPE_ROM:
						screen = screen_option;
						break;
					default:
						action = action_test;
						break;
					}
					break;
				case action_redo:
					action = action_test;
					break;
				}
			}
			// ESCape to menu
			if (buttons[button_escape].contains(worker.x, worker.y)) {
				task = task_menu;
				screen = screen_menu;
			}
			break;
		case screen_option:
			if (board.input(worker.x, worker.y)) {
				uint8_t tile = board.tile();
				board.toggle(tile);
				if (((ic.type == TYPE_FRAM) || (ic.type == TYPE_FLASH)) && board.isChecked(tile)) {
					switch (tile) {
					case tile_fram_serial:
					case tile_fram_file:
						board.reset(tile_fram_erase);
						board.reset(tile_fram_write);
						break;
					case tile_fram_erase:
						board.reset(tile_fram_serial);
						board.reset(tile_fram_file);
						board.reset(tile_fram_write);
						break;
					case tile_fram_write:
						board.reset(tile_fram_serial);
						board.reset(tile_fram_file);
						board.reset(tile_fram_erase);
						break;
					}
				}
				// UI debounce
				delay(100);
			}
			// CONTINUE
			if (buttons[button_continue].contains(worker.x, worker.y)) {
				if ((ic.type == TYPE_FRAM) || (ic.type == TYPE_FLASH)) {
					action = action_option;
					options[option_serial] = board.isChecked(tile_fram_serial);
					options[option_file] =  board.isChecked(tile_fram_file);
					options[option_erase] = board.isChecked(tile_fram_erase);
					options[option_write] = board.isChecked(tile_fram_write);
					// Read to serial or to file
					if (options[option_serial] || options[option_file]) {
						// Done in ROM loop
						memory.loop = &ic_rom_loop;
						screen = screen_memory;
					}
					// Erase or write
					if (options[option_erase] || options[option_write]) {
						if ((ic.type == TYPE_FRAM)) {
							// Done in FRAM loop for FRAM
							memory.loop = &ic_fram_loop;
						} else {
							// Done in Flash loop for Flash
							memory.loop = &ic_flash_loop;
						}
						if (options[option_erase]) {
							screen = screen_memory;
						} else {
							// Display file browser
							screen = screen_browse;
							action = action_browse;
						}
					}
				} else {
					// ROM options
					options[option_blank] = board.isChecked(tile_rom_blank);
					options[option_serial] = board.isChecked(tile_rom_serial);
					options[option_file] =  board.isChecked(tile_rom_file);
					screen = screen_memory;
					action = action_option;
				}
			}
			// ESCape to menu
			if (buttons[button_escape].contains(worker.x, worker.y)) {
				task = task_menu;
				screen = screen_menu;
			}
			break;
		case screen_browse:
			if (board.input(worker.x, worker.y)) {
				uint8_t tile = board.tile();
				if (TILES_BROWSE.tile[tile].flags == TILE_FLAG_FOLDER) {
					sd_folder(tile);
				} else {
					// TODO need rework. It's easy to select the wrong file
					screen = screen_memory;
					action = action_option;
				}
				// UI debounce
				delay(100);
			}
			// ESCape to menu
			if (buttons[button_escape].contains(worker.x, worker.y)) {
				folders.clear();
				files.clear();
				task = task_menu;
				screen = screen_menu;
			}
			break;
		}
	}

	///////////////////////////////////////////
	// Keyboard and button, no screen change //
	///////////////////////////////////////////

	if (worker.screen == screen) {
		switch (worker.screen) {
		case screen_keyboard:
			if (keyboard.read(worker.x, worker.y, touched)) {
				if (worker.code.length() < INPUT_LENGTH) {
					worker.code += keyboard.key();
				}
				tft.setCursor(INPUT_X, INPUT_Y);
				tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
				tft.setTextSize(INPUT_SIZE);
				tft.print(worker.code);
			}
			for (uint8_t index = button_enter; index < button_escape; ++index) {
				if (ux_button_pressed(index, touched)) {
					// Enter
					if (index == button_enter) {
						if (worker.code.length()) {
							switch (worker.task) {
							case task_logic:
								screen = screen_logic;
								action = action_load;
								break;
							case task_memory:
								screen = screen_memory;
								action = action_load;
								break;
							}
						} else {
							task = task_menu;
							screen = screen_menu;
						}
					}
					// Clear
					if (index == button_clear) {
						tft.fillRect(INPUT_X, INPUT_Y, TFT_WIDTH - 2 - INPUT_X, INPUT_SIZE * 8, COLOR_BACKGROUND);
						worker.code = "";
					}
					// Delete
					if (index == button_del) {
						worker.code.remove(worker.code.length() - 1, 1);
						tft.fillRect(INPUT_X + (INPUT_SIZE * 6 * worker.code.length()), INPUT_Y, INPUT_SIZE * 6, INPUT_SIZE * 8, COLOR_BACKGROUND);
					}
					// UI debounce
					delay(100);
				}
			}
			break;
		case screen_logic:
			if ((worker.action == action_identified) && (ics.count() > 1)) {
				for (uint8_t index = button_previous; index < button_escape; ++index) {
					if (ux_button_pressed(index, touched)) {
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
		case screen_browse:
			if (worker.action == action_browse) {
				for (uint8_t index = button_root; index < button_escape; ++index) {
					if (ux_button_pressed(index, touched)) {
						uint16_t first = worker.first;
						// Root
						if (index == button_root) {
							if (!path.equals("/")) {
								path = "/";
								sd_browse();
								first = 0;
							}
						}
						// Up
						if (index == button_up) {
							int last = path.lastIndexOf("/");
							if (last > 0) {
								path.remove(last);
							} else if (!path.equals("/")) {
								path = "/";
							}
							sd_browse();
							first = 0;
						}
						// Previous
						if (index == button_previous) {
							if (first > 0) {
								first -= 4;
							}
						}
						// Next
						if (index == button_next) {
							if (first  < worker.last) {
								first += 4;
							}
						}
						if (worker.first != first) {
							worker.first = first;
							sd_display();
						}
						// UI debounce
						delay(100);
					}
				}
			}
			break;
		}
	}
	if ((worker.task != task || worker.screen != screen) || worker.action != action) {
		worker.task = task;
		worker.screen = screen;
		worker.action = action;
#if DEBUG
		Debug(F("["));
		Debug(TASKS[worker.task]);
		Debug("/");
		Debug(SCREENS[worker.screen]);
		Debug("/");
		Debug(ACTIONS[worker.action]);
		Debug("] free memory ");
		Debugln(freemem());
#endif
		ui_draw_screen();
	}
#if CAPTURE
	if (worker.interrupted) {
		sd_screen_capture();
		worker.interrupted = false;
	}
#endif
}
