#ifndef DVB021_ENTITY_H
#define DVB021_ENTITY_H

#include "common.hpp"
#include <string>

class Sprite {
public:
	static Sprite* from_yaml(yaml_parser_t* parser);

	virtual void load_texture(const std::string& filename) = 0;
};

class Entity {
public:
	/**
	 * Position in worldspace.
	 */
	const Vector2f& world_pos() const;

	/**
	 * Position on grid (truncated to first tile)
	 */
	const Vector2i grid_pos() const;

	const Sprite* sprite() const;
	const std::string name() const;

protected:
	Entity(const Vector2f& pos, const Blueprint* blueprint, unsigned int level);
	size_t level;

private:
	Vector2f pos;
	const Blueprint* blueprint;
};

class Building: public Entity {
public:
	/**
	 * Construct a new building using blueprint bp and place it at the tile given
	 * by pos.
	 */
	static Building* place_at_tile(const Vector2i& pos, const Blueprint* blueprint){
		Vector2f world(pos.x * 48, pos.y * 48);
		return new Building(world, blueprint);
	}

private:
	Building(const Vector2f& pos, const Blueprint* blueprint);
};

class Creep: public Entity {
public:
	/**
	 * Spawn new creep at world space coordinate given by pos.
	 */
	static Creep* spawn_at(const Vector2f& pos, const Blueprint* blueprint, unsigned int level){
		return new Creep(pos, blueprint, level);
	}

private:
	Creep(const Vector2f& pos, const Blueprint* blueprint, unsigned int level);
};

#endif /* DVB021_ENTITY_H */
