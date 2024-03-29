#ifndef DVB021_TILEMAP_H
#define DVB021_TILEMAP_H

#include "spawn.hpp"
#include "waypoint.hpp"

#include <stdint.h>
#include <string>
#include <vector>
#include <map>

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

	/**
	 * Map width * height
	 */
	size_t size() const;

	/**
	 * Width in tiles.
	 */
	size_t map_width() const;

	/**
	 * Height in tiles.
	 */
	size_t map_height() const;

	/**
	 * Width of a tile in pixels.
	 */
	size_t tile_width() const;

	/**
	 * Height of a tile in pixels.
	 */
	size_t tile_height() const;

	const std::string& title() const;
	unsigned int slots() const;
	int inner() const;

	const Tile& operator[](unsigned int i) const;
	const Tile& at(unsigned int x, unsigned int y) const;
	void reserve(const Vector2i& pos, const Vector2i& size);
	void unreserve(const Vector2i& pos, const Vector2i& size);
	const std::string& texture_filename() const;
	void set_dimensions(size_t w, size_t h);

	std::vector<Tile>::iterator begin();
	std::vector<Tile>::iterator end();
	std::vector<Tile>::const_iterator begin() const;
	std::vector<Tile>::const_iterator end() const;

	const std::map<std::string, Waypoint*>& waypoints() const;
	const std::map<std::string, Spawnpoint*>& spawnpoints() const;

protected:
	Tilemap(const std::string& filename);

private:
	class TilemapPimpl* pimpl;
};

#endif /* DVB021_TILEMAP_H */
