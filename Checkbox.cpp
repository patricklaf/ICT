// Checkbox
// © 2021 Patrick Lafarguette
#include "Checkbox.h"

#define BOX_WIDTH 24
#define BOX_HEIGHT 24

Checkbox::Checkbox() {
	_gfx = NULL;
	_x = 0;
	_y = 0;
	Colors(0xffff, 0x0000);
	Reset();
}

Checkbox::~Checkbox() {
}

void Checkbox::Initialize(Adafruit_GFX* gfx, unsigned int x, unsigned int y) {
	_gfx = gfx;
	_x = x;
	_y = y;
	Reset();
}

void Checkbox::Reset() {
	_state = false;
	_checked = false;
}

void Checkbox::Colors(uint16_t color, uint16_t background) {
	_color = color;
	_background = background;
}

void Checkbox::Draw() {
	_gfx->drawRect(_x, _y, BOX_WIDTH, BOX_HEIGHT, _color);
	_gfx->fillRect(_x + 1, _y + 1, BOX_WIDTH - 2, BOX_HEIGHT - 2, _background);
}

void Checkbox::Draw(const bool checked) {
	if (_checked != checked) {
		_checked = checked;
		_gfx->fillRect(_x + 2, _y + 2, BOX_WIDTH - 4, BOX_HEIGHT - 4, _checked ? _color : _background);
	}
}

bool Checkbox::Read(int16_t x, int16_t y, const bool pressed) {
	bool state = false;
	if ((x >= (int16_t)_x) && (x < (int16_t)(_x + BOX_WIDTH)) && (y >= (int16_t)_y) && (y < (int16_t)(_y + BOX_HEIGHT)) && pressed) {
		// Pressed
		state = true;
	}
	if (_state != state) {
		// Changed
		_state = state;
		if (_state) {
			// Pressed, there is a click
			Draw(!_checked);
		}
	}
	return state;
//	if ((x < (int16_t)_x) || (x >= (int16_t)(_x + BOX_WIDTH)) || (y < (int16_t)_y) || (y >= (int16_t)(_y + BOX_HEIGHT))) {
//		_state = false;
//		return false;
//	}
//	if (_state != pressed) {
//		_state = pressed;
//		if (_state) {
//			Draw(!_checked);
//		}
//	}
//	return true;
}

bool Checkbox::Checked() {
	return _checked;
}
