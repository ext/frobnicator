#ifndef DVB021_REGION_H
#define DVB021_REGION_H

#include "common.hpp"
#include <string>

class Region {
protected:
	Region();

public:
	int x() const { return _x; }
	int y() const { return _y; }
	int w() const { return _w; }
	int h() const { return _h; }
	const std::string& name() const { return _name; }

	/**
	 * Generete a random point within the region.
	 *
	 * @param size If given fits a AABB within the region.
	 */
	Vector2f random_point(const Vector2i& size = Vector2i(0,0)) const;

	/**
	 * Get the middle coordinates of this region.
	 */
	Vector2f middle() const;

	/**
	 * Tell if a point is inside region or not.
	 * @todo fix AABB class
	 * @param inside If true the AABB must the completely inside region.
	 */
	bool contains(const Vector2f& pos, const Vector2f& size, bool inside=false) const;

protected:
	virtual void set(const std::string& key, const std::string& value);
	void parse(yaml_parser_t* parser);

private:
	std::string _name;
	int _x;
	int _y;
	int _w;
	int _h;
};

#endif /* DVB021_REGION_H */
