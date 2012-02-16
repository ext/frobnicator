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
			float s;
			float t;
		} uv;
		int set: 1;
		int build : 1;
	};

	virtual ~Tilemap();

protected:
	Tilemap(const std::string& filename);

private:
	class TilemapPimpl* pimpl;
};

#endif /* DVB021_TILEMAP_H */
