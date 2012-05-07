#ifndef DVB021_ENTITY_H
#define DVB021_ENTITY_H

#include "common.hpp"
#include <string>

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

	/**
	 * Update entity. Should be called every frame.
	 */
	virtual void tick(){}

protected:
	Entity(const Vector2f& pos, const Blueprint* blueprint, unsigned int level);
	size_t level;
	Vector2f pos;

private:
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

	/**
	 * Mark what region it currently is in.
	 */
	Creep& set_region(const std::string& name);

	/**
	 * Get what region it is currently in.
	 * @return Empty string if outside any region.
	 */
	std::string get_region() const;

	/**
	 * Set where it is going.
	 */
	Creep& set_dst(const Vector2f& dst);

	virtual void tick();

private:
	Creep(const Vector2f& pos, const Blueprint* blueprint, unsigned int level);

	Vector2f dst;
	std::string region;
};

#endif /* DVB021_ENTITY_H */
