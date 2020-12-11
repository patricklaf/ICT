// Keyboard
// © 2020 Patrick Lafarguette

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <Adafruit_GFX.h>

class Keyboard {
public:
	Keyboard(Adafruit_GFX* gfx, unsigned int y = 0);
	virtual ~Keyboard();

	void size(uint8_t size);
	void colors(uint16_t color, uint16_t background);

	void draw();
	void draw(const bool pressed);

	bool read(int16_t x, int16_t y, const bool pressed);
	char key();

private:
	Adafruit_GFX* _gfx;
	uint16_t _y;
	uint8_t _size;
	uint16_t _dx;
	uint16_t _dy;
	uint16_t _color, _background;
	bool _state;
	uint8_t _key;
	uint8_t _none;
};

#endif /* KEYBOARD_H_ */
