// Checkbox
// © 2021 Patrick Lafarguette

#ifndef CHECKBOX_H_
#define CHECKBOX_H_

#include <Adafruit_GFX.h>

class Checkbox {
public:
	Checkbox();
	virtual ~Checkbox();
	void Initialize(Adafruit_GFX* gfx, unsigned int x = 0, unsigned int y = 0);
	void Reset();

	void Colors(uint16_t color, uint16_t background);

	void Draw();
	void Draw(const bool checked);

	bool Read(int16_t x, int16_t y, const bool pressed);
	bool Checked();

private:
	Adafruit_GFX* _gfx;
	uint16_t _x;
	uint16_t _y;
	uint8_t _size;
	uint16_t _color, _background;
	bool _state;
	bool _checked;
};

#endif /* CHECKBOX_H_ */
