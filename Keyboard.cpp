// Keyboard
// © 2020 Patrick Lafarguette

#include "Keyboard.h"
#include <String.h>

#define AZERTY 1
#define QWERTY 0

#if AZERTY
// 1 2 3 4 5 6 7 8 9 0
// a z e r t y u i o p
// q s d f g h j k l m
// w x c v b n
const char KEYS[] = "1234567890azertyuiopqsdfghjklmwxcvbn";
#endif

#if QWERTY
// 0 1 2 3 4 5 6 7 8 9
// q w e r t y u i o p
// a s d f g h j k l
// m z x c v b n
const char KEYS[] = "1234567890qwertyuiopasdfghjkl zxcvbnm";
#endif

#define KEY_ROWS 4
#define KEY_COLUMNS 10
#define KEY_WIDTH 32
#define KEY_HEIGHT 32

Keyboard::Keyboard(Adafruit_GFX* gfx, unsigned int y) {
	_gfx = gfx;
	_y = y;
	_state = false;
	_key = _none = strlen(KEYS);
	size(2);
	colors(0xffff, 0x0000);
}

Keyboard::~Keyboard() {
}

void Keyboard::size(uint8_t size) {
	_size = size;
	_dx = KEY_WIDTH / 2 - 6 * _size / 2;
	_dy = KEY_HEIGHT / 2 - 8 * _size / 2;
}

void Keyboard::colors(uint16_t color, uint16_t background) {
	_color = color;
	_background = background;
}

void Keyboard::draw() {
	uint8_t key = 0;
	_gfx->setTextSize(_size);
	_gfx->setTextColor(_color, _background);

	for (uint8_t row = 0; row < KEY_ROWS; ++row) {
		for (uint8_t column = 0; column < KEY_COLUMNS; ++column) {
			if (KEYS[key] != ' ') {
				_gfx->fillRect(column * KEY_WIDTH + 1, _y + row * KEY_HEIGHT + 1, KEY_WIDTH - 2, KEY_HEIGHT - 2, _background);
				_gfx->setCursor(column * KEY_WIDTH + _dx, _y + row * KEY_HEIGHT + _dy);
				_gfx->print(KEYS[key]);
			}
			if (++key == _none) {
				return;
			}
		}
	}
}

void Keyboard::draw(const bool pressed) {
	if (_key < _none) {
		_gfx->setTextSize(_size);
		if (pressed) {
			_gfx->setTextColor(_background, _color);
		} else {
			_gfx->setTextColor(_color, _background);
		}
		uint8_t row = _key / KEY_COLUMNS;
		uint8_t column = _key % KEY_COLUMNS;
		_gfx->fillRect(column * KEY_WIDTH + 1, _y + row * KEY_HEIGHT + 1, 30, 30, pressed ? _color : _background);
		_gfx->setCursor(column * KEY_WIDTH + _dx, _y + row * KEY_HEIGHT + _dy);
		_gfx->print(KEYS[_key]);
	}
}

bool Keyboard::read(int16_t x, int16_t y, const bool pressed) {
	if (x < 0 || x >= _gfx->width()) {
		return false;
	}
	uint16_t row = ( y - _y) / KEY_HEIGHT;
	uint16_t column = x / KEY_WIDTH;
	uint8_t key = row * 10 + column;

	if (_state != pressed) {
		_state = pressed;
		if (_state) {
			// Key just pressed
			if ((key < _none) && (KEYS[key] != ' ')) {
				_key = key;
				draw(true);
			} else {
				_key = _none;
			}
		} else {
			// Key just released
			draw(false);
//			return (key < _none) && (KEYS[key] != ' ');
			return (_key < _none) && (KEYS[_key] != ' ');
		}
	} else {
		if (_state) {
			// Key pressed
			if (key != _key) {
				// Key change
				draw(false);
				if ((key < _none) && (KEYS[key] != ' ')) {
					_key = key;
					draw(true);
				} else {
					_key = _none;
				}
			}
		}
	}
	return false;
}

char Keyboard::key() {
	return KEYS[_key];
}
