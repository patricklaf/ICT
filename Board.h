// Board
// © 2021 Patrick Lafarguette

#ifndef BOARD_H_
#define BOARD_H_

#include <Adafruit_GFX.h>

#define TILE_FLAG_NONE   (0)
#define TILE_FLAG_CENTER (1 << 0)
#define TILE_FLAG_CHECK  (1 << 1)
#define TILE_FLAG_FILE   (1 << 2)
#define TILE_FLAG_FOLDER (1 << 3)

typedef struct Tile {
	uint8_t flags;
	const char* label;
} Tile;

typedef struct Tiles {
	uint8_t count;
	uint8_t rows;
	uint8_t columns;
	Tile tile[];
} Tiles;

class Board {
public:
	Board(Adafruit_GFX* gfx, unsigned int x = 0, unsigned int y = 0, unsigned int w = 0, unsigned int h = 0);
	virtual ~Board();

	void tiles(Tiles& tiles);
	void size(uint8_t size);
	void colors(uint16_t color, uint16_t background);

	void draw();
	void check(uint8_t tile);

	bool input(int16_t x, int16_t y);

	uint8_t tile();

	bool isChecked(uint8_t tile);
	void toggle(uint8_t tile);
	void set(uint8_t tile);
	void reset(uint8_t tile);
private:
	Adafruit_GFX* _gfx;
	Tiles* _tiles;
	// Board position and dimension
	uint16_t _x;
	uint16_t _y;
	uint16_t _w;
	uint16_t _h;
	// Touched tile
	uint8_t _tile;
	// Text size and offset
	uint8_t _size;
	uint16_t _dx;
	uint16_t _dy;
	// Tile width and height
	uint16_t _width;
	uint16_t _height;
	// Colors
	uint16_t _color, _background;
};

#endif /* BOARD_H_ */
