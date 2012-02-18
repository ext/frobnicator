#ifndef DVB021_TILEMAP_H
#define DVB021_TILEMAP_H

#include <string>
#include <vector>

class Tilemap {
public:
	/**
	 * Tile data.
	 */
	struct Tile {
		float uv[8]; /* float uv[2][4] */

		uint8_t x;
		uint8_t y;
		uint16_t index;

		uint8_t set: 1;
		uint8_t build: 1;
	};

	virtual ~Tilemap();

	size_t size() const;
	size_t width() const;
	const Tile& operator[](unsigned int i) const;
	const Tile& at(unsigned int x, unsigned int y) const;
	const std::string& texture_filename() const;

	std::vector<Tile>::const_iterator begin() const;
	std::vector<Tile>::const_iterator end() const;

protected:
	Tilemap(const std::string& filename);

private:
	class TilemapPimpl* pimpl;
};

#endif /* DVB021_TILEMAP_H */
