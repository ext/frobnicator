#ifndef DVB021_TILEMAP_H
#define DVB021_TILEMAP_H

#include <string>

class Tilemap {
public:
	/**
	 * Tile data.
	 */
	struct Tile {
		struct {
			/* why yes this is bad variable naming and will confuse people */
			float s; /* min */
			float t;
			float u; /* max */
			float v;
		} uv;
		uint8_t index;
		uint8_t set: 1;
		uint8_t build: 1;
	};

	virtual ~Tilemap();

	size_t size() const;
	size_t width() const;


protected:
	Tilemap(const std::string& filename);

private:
	class TilemapPimpl* pimpl;
};

#endif /* DVB021_TILEMAP_H */
