// IC Tester
// © 2020 Patrick Lafarguette
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

#include "Stack.h"
#include "TMS41xx.h"

// TFT calibration
// See TouchScreen_Calibr_native example in MCUFRIEND_kbv library
const int XP = 8, XM = A2, YP = A3, YM = 9;
const int TS_LEFT = 908, TS_RT = 122, TS_TOP = 85, TS_BOT = 905;

MCUFRIEND_kbv tft;
// XP (LCD_RS), XM (LCD_D0) resistance is 345 Ω
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 345);

// SD
SdFatSoftSpi<12, 11, 13> fat;

typedef struct IC {
	unsigned int pins;
	String code;
	String description;
} IC;

Stack<IC*> ics;
Stack<String*> lines;

#define DEBUG 0 // 1 to enable serial debug messages, 0 to disable

#if DEBUG
#define Debug(...) Serial.print(__VA_ARGS__)
#define Debugln(...) Serial.println(__VA_ARGS__)
#else
#define x(...)
#define Debug(...)
#define Debugln(...)
#endif

#define CAPTURE 0 // 1 to enable screen captures,  0 to disable

typedef enum states {
	state_startup,
	state_media,
	state_menu,
	state_identify_logic,
	state_test_logic,
	state_test_ram,
	state_package_dip14,
	state_package_dip16,
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
	button_previous = 0,
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
	unsigned int pins;
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

// Package pins
const unsigned int DIP14[14] = { 30, 32, 34, 36, 38, 40, 42, 43, 41, 39, 37, 35, 33, 31 };
const unsigned int DIP16[16] = { 30, 32, 34, 36, 38, 40, 42, 44, 45, 43, 41, 39, 37, 35, 33, 31 };

// Database filename
#define FILENAME "database.txt"

///////////////////////
// TMS4164, TMS41256 //
///////////////////////

const unsigned int TMS41xx_Ax[] = {
	38, // A0
	42, // A1
	40, // A2
	39, // A3
	41, // A4
	43, // A5
	37, // A6
	45, // A7
	30, // A8 TMS41256
};

#define TMS41xx_RAS 36
#define TMS41xx_CAS 33
#define TMS41xx_WE 34
#define TMS41xx_D 32
#define TMS41xx_Q 35
#define TMS41xx_GND 31
#define TMS41xx_VCC 44

#define TMS4164_BITS 8
#define TMS41256_BITS 9

void ic_tms41xx_address(unsigned int address, const unsigned int bits) {
	for (unsigned int bit = 0; bit < bits; ++bit) {
		digitalWrite(TMS41xx_Ax[bit], address & HIGH);
		address >>= 1;
	}
}

void ic_tms41xx_write(TMS41xx& tms41xx) {
	// Row address
	ic_tms41xx_address(tms41xx.row, tms41xx.bits);
	digitalWrite(TMS41xx_RAS, LOW);
	// WE
	digitalWrite(TMS41xx_WE, LOW);
	// D
	digitalWrite(TMS41xx_D, tms41xx.d);
	// Column address
	ic_tms41xx_address(tms41xx.column, tms41xx.bits);
	// Idle
	digitalWrite(TMS41xx_CAS, LOW);
	digitalWrite(TMS41xx_WE, HIGH);
	digitalWrite(TMS41xx_RAS, HIGH);
	digitalWrite(TMS41xx_CAS, HIGH);
}

void ic_tms41xx_read(TMS41xx& tms41xx) {
	// Row address
	ic_tms41xx_address(tms41xx.row, tms41xx.bits);
	digitalWrite(TMS41xx_RAS, LOW);
	// Column address
	ic_tms41xx_address(tms41xx.column, tms41xx.bits);
	digitalWrite(TMS41xx_CAS, LOW);
	// Q
	tms41xx.q = digitalRead(TMS41xx_Q);
	// Idle
	digitalWrite(TMS41xx_RAS, HIGH);
	digitalWrite(TMS41xx_CAS, HIGH);
}

void ic_tms41xx_fill(TMS41xx& tms41xx) {
	worker.color = COLOR_GOOD;
	for (tms41xx.row = 0; tms41xx.row < tms41xx.high; ++tms41xx.row) {
		worker.success = 0;
		worker.failure = 0;
		for (tms41xx.column = 0; tms41xx.column < tms41xx.high; ++tms41xx.column) {
			ic_tms41xx_write(tms41xx);
			ic_tms41xx_read(tms41xx);
			if (tms41xx.d != tms41xx.q) {
				worker.failure++;
				worker.color = COLOR_BAD;
			} else {
				worker.success++;
			}
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

void ic_tms41xx_alternate(TMS41xx& tms41xx) {
	worker.color = COLOR_GOOD;
	for (tms41xx.row = 0; tms41xx.row < tms41xx.high; ++tms41xx.row) {
			worker.success = 0;
			worker.failure = 0;
			for (tms41xx.column = 0; tms41xx.column < tms41xx.high; ++tms41xx.column) {
			ic_tms41xx_write(tms41xx);
			ic_tms41xx_read(tms41xx);
			if (tms41xx.d != tms41xx.q) {
				worker.failure++;
				worker.color = COLOR_BAD;
			} else {
				worker.success++;
			}
			tms41xx.d ^= HIGH;
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

void ic_tms41xx(TMS41xx& tms41xx) {
	// UI
	worker.ram = 0;
	worker.color = TFT_WHITE;
	for (unsigned int index = 0; index < 4; ++index) {
		ui_draw_ram();
	}
	worker.ram = 0;
#if DEBUG
	// Timer
	unsigned long start = millis();
#endif
	// Setup
	tms41xx.high = 1 << tms41xx.bits;
	for (unsigned int index = 0; index < 16; ++index) {
		pinMode(DIP16[index], DIP16[index] == TMS41xx_Q ? INPUT : OUTPUT);
	}
	// VCC, GND
	digitalWrite(TMS41xx_GND, LOW);
	digitalWrite(TMS41xx_VCC, HIGH);
	// Idle
	digitalWrite(TMS41xx_WE, HIGH);
	digitalWrite(TMS41xx_RAS, HIGH);
	digitalWrite(TMS41xx_CAS, HIGH);
	for (unsigned int bit = 0; bit < tms41xx.bits; ++bit) {
		digitalWrite(TMS41xx_RAS, LOW);
		digitalWrite(TMS41xx_RAS, HIGH);
	}
	// Test
	worker.indicator = 0;
	Debugln(F("Alternate LOW"));
	tms41xx.d = LOW;
	ic_tms41xx_alternate(tms41xx);
	Debugln(F("Alternate HIGH"));
	tms41xx.d = HIGH;
	ic_tms41xx_alternate(tms41xx);
	Debugln(F("Fill LOW"));
	tms41xx.d = LOW;
	ic_tms41xx_fill(tms41xx);
	Debugln(F("Fill HIGH"));
	tms41xx.d = HIGH;
	ic_tms41xx_fill(tms41xx);
	// Reset VCC
	digitalWrite(TMS41xx_VCC, LOW);
#if DEBUG
	unsigned long stop = millis();
	Debug("Time elapsed");
	Debug((stop - start) / 1000.0);
#endif
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
	Debug(F("TFT initialized 0x"));
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
			if ((tft.getCursorX() + (index - start) * 6 * size) > TFT_WIDTH) {
				tft.println();
			}
			++index;
			tft.print(string.substring(start, index));
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
		ui_draw_center(F("version 1.2.0"), tft.getCursorY());
		ui_draw_center(F("01/09/2020"), tft.getCursorY());
		// Author
		tft.setTextColor(TFT_WHITE);
		ui_draw_center(F("Patrick Lafarguette"), AREA_CONTENT + 32);
		ui_draw_escape();
		break;
	case state_media:
		ui_draw_header(true);
		// Error
		ui_draw_error(F("Insert an SD card"));
		ui_draw_escape();
		break;
	case state_menu: {
		worker.action = action_idle;
		const char* items[] = { "identify logic", "test logic", "test RAM" };
		ui_draw_menu(items, 3);
	}
#if CAPTURE
		sd_screen_capture();
#endif
		break;
	case state_identify_logic: {
		const char* items[] = { "DIP 14 package", "DIP 16 package" };
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
		ui_draw_center(F("Identify logic"), tft.getCursorY());
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
		lines.clear();
		ui_draw_header(true);
		// Action
		tft.setTextSize(2);
		ui_draw_center(F("Test logic"), tft.getCursorY());
		ic_test_logic();
		break;
	case state_test_ram:
		ui_draw_header(true);
		// Action
		tft.setTextSize(2);
		ui_draw_center(F("Test RAM"), tft.getCursorY());
		ic_test_ram();
		break;
	case state_identified:
		ui_clear_footer();
		switch (ics.count()) {
			case 0:
				ui_draw_content();
				/* no break */
			case 1:
				break;
			default:
				// Add navigator
				buttons[button_previous].initButton(&tft, BUTTON_X,
						BUTTON_Y + 2 * (BUTTON_H + BUTTON_SPACING_Y), BUTTON_W, BUTTON_H,
						COLOR_KEY, COLOR_KEY, COLOR_LABEL, (char*)"<", BUTTON_TEXTSIZE);
				buttons[button_previous].drawButton();
				buttons[button_next].initButton(&tft, BUTTON_X + (BUTTON_W + BUTTON_SPACING_X),
						BUTTON_Y + 2 * (BUTTON_H + BUTTON_SPACING_Y),  BUTTON_W, BUTTON_H,
						COLOR_KEY, COLOR_KEY, COLOR_LABEL, (char*)">", BUTTON_TEXTSIZE);
				buttons[button_next].drawButton();
				break;
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
					tft.print(F(" cycles, "));
					tft.print(percent);
					tft.println(F("% passed"));
					tft.setTextSize(2);
					tft.setTextColor(worker.success ? (worker.failure ? COLOR_MIXED : COLOR_GOOD) : COLOR_BAD);
					tft.println(worker.success ? (worker.failure ? F("Unreliable") : F("Good!!!")) : F("Bad!!!"));
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
			ui_draw_error(F("No match found"));
			return;
		case 1:
			tft.println(F("Match found"));
			break;
		default:
			tft.print(F("Match found "));
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
			ui_draw_error(F("No match found"));
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

bool ic_test_logic(String line) {
	bool result = true;
	int clk = -1;
	Debugln(line);
	const unsigned int* pins =  NULL;
	switch (worker.pins) {
	case 14:
		pins = DIP14;
		break;
	case 16:
		pins = DIP16;
		break;
	}
	// VCC, GND and output
	for (unsigned int index = 0; index < worker.pins; ++index) {
		switch (line[index]) {
		case 'V':
			pinMode(pins[index], OUTPUT);
			digitalWrite(pins[index], HIGH);
			break;
		case 'G':
			pinMode(pins[index], OUTPUT);
			digitalWrite(pins[index], LOW);
			break;
		case 'L':
			digitalWrite(pins[index], LOW);
			pinMode(pins[index], INPUT_PULLUP);
			break;
		case 'H':
			digitalWrite(pins[index], LOW);
			pinMode(pins[index], INPUT_PULLUP);
			break;
		}
	}
	delay(5);
	// Write
	for (unsigned int index = 0; index < worker.pins; ++index) {
		switch (line[index]) {
		case 'X':
		case '0':
			pinMode(pins[index], OUTPUT);
			digitalWrite(pins[index], LOW);
			break;
		case '1':
			pinMode(pins[index], OUTPUT);
			digitalWrite(pins[index], HIGH);
			break;
		case 'C':
			clk = pins[index];
			pinMode(pins[index], OUTPUT);
			digitalWrite(pins[index], LOW);
			break;
		}
	}
	// Clock
	if (clk != -1) {
		pinMode(clk, INPUT_PULLUP);
		delay(10);
		pinMode(clk, OUTPUT);
		digitalWrite(clk, LOW);
	}
	delay(5);
	// Read
	for (unsigned int index = 0; index < worker.pins; index++) {
		switch (line[index]) {
		case 'H':
			if (!digitalRead(pins[index])) {
				result = false;
				Debug(F("L"));
			} else {
				Debug(F(" "));
			}
			break;
		case 'L':
			if (digitalRead(pins[index])) {
				result = false;
				Debug(F("H"));
			} else {
				Debug(F(" "));
			}
			break;
		default:
			Debug(F(" "));
		}
	}
	Debugln();
	return result;
}

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
			ic.pins = file.readStringUntil('\n').toInt();
			if (worker.pins == ic.pins) {
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
			ic.pins = file.readStringUntil('\n').toInt();
			if (worker.code.toInt() == ic.code.toInt()) {
				worker.found = true;
				worker.index = 0;
				worker.success = 0;
				worker.failure = 0;
				worker.pins = ic.pins;
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

void ic_test_ram() {
	IC ic;
	TMS41xx tms41xx = { 0, 0, 0, 0, 0, 0 };

	worker.found = true;
	switch (tms41xx_identify(worker.code.toInt())) {
	case TMS4164:
		ic.code = worker.code;
		ic.description = "65,536 x 1 dynamic random-access memory";
		tms41xx.bits = TMS4164_BITS;
		break;
	case TMS41256:
		ic.code = worker.code;
		ic.description = "262,144 x 1 dynamic random-access memory";
		tms41xx.bits = TMS41256_BITS;
		break;
	default:
		worker.found = false;
	}
	if (worker.found) {
		ics.push(new IC(ic));
		ui_draw_content();
		ic_tms41xx(tms41xx);
	}
	worker.state = state_tested;
	ui_draw_screen();
}

/////////////
// Arduino //
/////////////

void setup() {
	Serial.begin(115200);
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
				case action_test_logic:
				case action_test_ram:
					if (buttons[button_redo].contains(worker.x, worker.y)) {
						Debugln("redo");
						state = worker.action == action_test_logic ? state_test_logic : state_test_ram;
					}
					break;
			}
			if (buttons[button_escape].contains(worker.x, worker.y)) {
				Debugln("menu");
				state = state_menu;
			}
			break;
		case state_media:
			if (buttons[button_escape].contains(worker.x, worker.y)) {
				Debugln("media");
				if (fat.begin(10)) {
					state = state_menu;
				} else {
					Debugln("Error SD");
				}
			}
			break;
		case state_menu:
			if (buttons[button_identify_logic].contains(worker.x, worker.y)) {
				Debugln("Identify logic");
				state = state_identify_logic;
			}
			if (buttons[button_test_logic].contains(worker.x, worker.y)) {
				Debugln("Test logic");
				ics.clear();
				worker.action = action_test_logic;
				worker.code = "";
				state = state_keyboard;
			}
			if (buttons[button_test_ram].contains(worker.x, worker.y)) {
				Debugln("Test RAM");
				ics.clear();
				worker.action = action_test_ram;
				worker.code = "";
				state = state_keyboard;
			}
			break;
		case state_identify_logic:
			if (buttons[button_dip14].contains(worker.x, worker.y)) {
				Debugln("DIP 14");
				state = state_package_dip14;
				worker.pins = 14;
			}
			if (buttons[button_dip16].contains(worker.x, worker.y)) {
				Debugln("DIP 16");
				state = state_package_dip16;
				worker.pins = 16;
			}
			if (buttons[button_escape].contains(worker.x, worker.y)) {
				Debugln("menu");
				state = state_menu;
			}
			break;
		}
	}
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
	if (worker.state != state) {
#if CAPTURE_STATE
		sd_screen_capture();
#endif
		worker.state = state;
		ui_draw_screen();
		Debug("State: ");
		Debugln(worker.state);
	}
}
