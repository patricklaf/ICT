// Board
// © 2021 Patrick Lafarguette

#include "Board.h"

static const unsigned char PROGMEM icon_file[32] = {
	0x00, 0x00,
	0x00, 0x00,
	0x07, 0xc0,
	0x0c, 0x40,
	0x1c, 0x40,
	0x3c, 0x40,
	0x20, 0x40,
	0x20, 0x40,
	0x20, 0x40,
	0x20, 0x40,
	0x20, 0x40,
	0x20, 0x40,
	0x20, 0x40,
	0x3f, 0xc0,
	0x00, 0x00,
	0x00, 0x00
};

static const unsigned char PROGMEM icon_folder[32] = {
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x03, 0xe0,
	0x04, 0x20,
	0x7f, 0xe0,
	0x40, 0x20,
	0x40, 0x20,
	0x40, 0x20,
	0x40, 0x20,
	0x40, 0x20,
	0x40, 0x20,
	0x7f, 0xe0,
	0x00, 0x00,
	0x00, 0x00
};

Board::Board(Adafruit_GFX* gfx, unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
	_gfx = gfx;
	_x = x;
	_y = y;
	_w = w;
	_h = h;
	_tiles = nullptr;
	colors(0xffff, 0x0000);
}

Board::~Board() {
}

void Board::tiles(Tiles& tiles) {
	_tiles = (Tiles*)&tiles;
}

void Board::size(uint8_t size) {
	_size = size;
	_width = _w / _tiles->columns;
	_height = _h / _tiles->rows;
	_dx = _width / 2 - 6 * _size / 2;
	_dy = _height / 2 - 8 * _size / 2;
}

void Board::colors(uint16_t color, uint16_t background) {
	_color = color;
	_background = background;
}

void Board::draw() {
	uint8_t tile = 0;
	_gfx->setTextSize(_size);
	_gfx->setTextColor(_color, _background);

	for (uint8_t row = 0; row < _tiles->rows; ++row) {
		for (uint8_t column = 0; column < _tiles->columns; ++column) {
			// Draw background
			_gfx->fillRect(_x + column * _width + 1, _y + row * _height + 1, _width - 2, _height - 2, _background);
			if (_tiles->tile[tile].label) {
				uint8_t dx = 8;
				if (_tiles->tile[tile].flags & TILE_FLAG_CENTER) {
					dx = _width / 2 - 6 * _size * strlen(_tiles->tile[tile].label) / 2;
				}
				if (_tiles->tile[tile].flags & (TILE_FLAG_FILE | TILE_FLAG_FOLDER)) {
					_gfx->drawBitmap(_x + column * _width + 4, _y + row * _height + _dy,
							_tiles->tile[tile].flags & TILE_FLAG_FILE ? icon_file : icon_folder, 12, 16, _color, _background);
					dx += 12;
				}
				_gfx->setCursor(_x + column * _width + dx, _y + row * _height + _dy);
				_gfx->print(_tiles->tile[tile].label);
				check(tile);
			}
			if (++tile == _tiles->count) {
				return;
			}
		}
	}
}

void Board::check(uint8_t tile) {
	uint8_t row = tile / _tiles->columns;
	uint8_t column = tile % _tiles->columns;
	uint16_t x = _x + (column + 1) * _width - 2;
	uint16_t y = _y + (row + 1) * _height - 2;
	uint16_t dy = _height / 2;
	_gfx->fillTriangle(x, y, x - dy, y, x, y - dy, isChecked(tile) ? 0x07E0 : _background);
}

bool Board::input(int16_t x, int16_t y) {
	if (x < (int16_t)_x || x >= (int16_t)(_x + _w) || y < (int16_t)_y || y >= (int16_t)(_y + _h)) {
		return false;
	}
	uint16_t row = ( y - _y) / _height;
	uint16_t column = (x - _x) / _width;
	uint8_t tile = row * _tiles->columns + column;

	if ((tile < _tiles->count) && (_tiles->tile[tile].label)) {
		_tile = tile;
		return true;
	}
	return false;
}

uint8_t Board::tile() {
	return _tile;
}

bool Board::isChecked(uint8_t tile) {
	 return _tiles->tile[tile].flags & TILE_FLAG_CHECK;
}

void Board::toggle(uint8_t tile) {
	_tiles->tile[tile].flags ^= TILE_FLAG_CHECK;
	check(tile);
}

void Board::set(uint8_t tile) {
	_tiles->tile[tile].flags |= TILE_FLAG_CHECK;
	check(tile);
}

void Board::reset(uint8_t tile) {
	_tiles->tile[tile].flags &= ~TILE_FLAG_CHECK;
	check(tile);
}
