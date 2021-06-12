// ICT
// © 2021 Patrick Lafarguette

#ifndef ICT_H_
#define ICT_H_

#include <Arduino.h>

#define ICT_VERSION "2.4.0"
#define ICT_DATE "12/06/2021"

class Filename {
public:
	Filename(char *filename) {
		_filename = strdup(filename);
		if (_filename == nullptr) {
			Serial.println("can not allocate");
		}
	}

	~Filename() {
		free(_filename);
	}

	const char* Buffer() {
		return _filename;
	}

private:
	char* _filename;
};

// UI size
#define TFT_WIDTH 320
#define TFT_HEIGHT 240

// UI areas
#define AREA_HEADER 0
#define AREA_CONTENT 60 // 80
#define AREA_FOOTER 180

// UI buttons
#define BUTTON_X 33
//#define BUTTON_Y 90
#define BUTTON_Y 214
#define BUTTON_W 52
//#define BUTTON_H 48
#define BUTTON_H 44
#define BUTTON_SPACING_X 12
#define BUTTON_SPACING_Y 12
#define BUTTON_TEXTSIZE 2

// UI input
#define INPUT_HEIGHT 54
#define INPUT_X 8
#define INPUT_Y 12
#define INPUT_SIZE 4
#define INPUT_LENGTH 12

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

// Tasks
typedef enum tasks_t {
	task_startup,
	task_media,
	task_menu,
	task_logic,
	task_memory,
} tasks_t;

const char* TASKS[] = {
		"startup",
		"media",
		"menu",
		"logic",
		"memory",
};

// Screens
typedef enum screens_t {
	screen_startup,
	screen_media,
	screen_menu,
	screen_logic,
	screen_memory,
	screen_keyboard,
	screen_option,
	screen_browse,
} screens_t;

const char* SCREENS[] = {
		"startup",
		"media",
		"menu",
		"logic",
		"memory",
		"keyboard",
		"option",
		"browse",
};

// Actions
typedef enum actions_t {
	action_idle,
	action_identify,
	action_identified,
	action_keyboard,
	action_load,
	action_reload,
	action_menu,
	action_option,
	action_test,
	action_redo,
	action_browse,
} actions_t;

const char* ACTIONS[] = {
		"idle",
		"identify",
		"identified",
		"keyboard",
		"load",
		"reload",
		"menu",
		"option",
		"test",
		"write",
		"redo",
		"browse",
};

// Buttons
typedef enum buttons_t {
	// Continue
	button_continue = 0,
	// Redo
	button_redo = 0,
	// Keyboard
	button_enter = 0,
	button_clear,
	button_del,
	// Identified
	button_test = 0,
	// Browse
	button_root = 0,
	button_up,
	// Navigator
	button_previous,
	button_next,
	button_escape,
	button_count,
} buttons_t;

// Tiles
typedef enum tiles_t {
	// Menu
	tile_logic,
	tile_memory,
	// Logic
	tile_dip14 = 0,
	tile_dip16,
	tile_dip18,
	tile_dip20,
	tile_dip22,
	tile_dip24,
	tile_dip28,
	tile_dip32,
	tile_dip40,
	tile_keyboard,
	// ROM
	tile_rom_blank = 0,
	tile_rom_serial,
	tile_rom_file,
	// FRAM, FLASH
	tile_fram_serial = 0,
	tile_fram_file,
	tile_fram_erase,
	tile_fram_write,
} tiles_t;

// Options
typedef enum options_t {
	option_blank,
	option_serial,
	option_file,
	option_erase,
	option_write,
	option_count,
} options_t;

#endif /* ICT_H_ */
