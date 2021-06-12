// Keyboard
// © 2020-2021 Patrick Lafarguette

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <Adafruit_GFX.h>

class Keyboard {
public:
	Keyboard(Adafruit_GFX* gfx, unsigned int x = 0, unsigned int y = 0, unsigned int w = 0, unsigned int h = 0);
	virtual ~Keyboard();

	void size(uint8_t size);
	void colors(uint16_t color, uint16_t background);

	void draw();
	void draw(const bool pressed);

	bool read(int16_t x, int16_t y, const bool pressed);
	char key();

private:
	Adafruit_GFX* _gfx;
	// Keyboard position and dimension
	uint16_t _y;
	// Text size and offset
	uint8_t _size;
	uint16_t _dx;
	uint16_t _dy;
	bool _state;
	// Touched key
	uint8_t _key;
	uint8_t _none;
	// Colors
	uint16_t _color, _background;
};

#endif /* KEYBOARD_H_ */
